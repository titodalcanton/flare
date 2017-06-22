import fcntl
import os
import sys
import platform
import argparse
import flare_submit
import astropy.units as units
from astropy.cosmology import Planck15 as cosmo
import math
import numpy
import subprocess
import time
import string

#This script controls runs for a multidimensional parameter space survey, 
#The survey compares Fisher v Bayes, mergers v not and higher-modes v 22 only. 
#Progress is recorded and new runs are issued based on a status list file

#First step is setting up the parameter cases:
Ms=[ "1e2", "1e4", "1e6" ] 
qs=[ "2", "8" ]
incs=[ "12", "03", "02" ]
zs=[ "1", "4", "20" ]
#Corresponding distances from astropy.cosmology.Planck15, in Mpc
lum_dist={"1":6791.811049825756, "4":36697.04066923789, "20":230421.93422332808}
modes=[ "lm2", "all" ]
wfs= [ "full", "insp" ]
#orientation: ref (lambda=3pi/4,phi0=pi/3,pol=pi/3,beta=pi/3)
codes=[ "b", "p" ]

print "run_control invoked with args: ",sys.argv
#parse ars
parser = argparse.ArgumentParser(description="Control runs for parameter space survey");
parser.add_argument('name',help="The basename for the status list")
parser.add_argument('-g',help="Generate a new status list (must not exist).",action="store_true")
parser.add_argument('-m',help="select modes option "+str(modes),default=modes[0])
parser.add_argument('-z',help="select redshift option "+str(zs),default=zs[0])
parser.add_argument('-c',help="select code option "+str(codes),default=codes[0])
parser.add_argument('-s',help="status to seek and operate on.",default="new")
parser.add_argument('-f',help="comma-separated extra flags for flare_submit.",default="")
parser.add_argument('--set',help="set status of tag to stat.",nargs=2,metavar=("tag","stat"))
parser.add_argument('-x',help="expect or require this status for set.",default="")

args=parser.parse_args()

extra_flags= string.replace(args.f,","," ")
print "pass-through flags: '",extra_flags,"'"

#A fresh list of runs is generated by: generate_status_list
#The list will contain run_tag - status pairs
#These are processed by get_next_tag, write_status
#Status list  access is controlled so that only one process can access the file at a time. 
tags = ["new","processing","submitted","running","need_restart","need_analysis","done"]
def read_file_data(fd):
    data=[]
    for line in fd:
        row=(line[:-1].split())
        data.append(row)
    return data

def write_file_data(fd,data):
    for row in data:
        #print "row=",row
        for s in row:
            fd.write(s+" ")
        fd.write("\n")

def generate_tag(m,q,z,inc,mode,wf,code):
    return m+"_"+q+"_"+z+"_"+inc+"_"+mode+"_"+wf+"_"+code+"_p"

def read_tag(tag):
    keys=["M","q","z","inc","modes","wf","code","pars"]
    vals=tag.split('_')
    return dict(zip(keys,vals))

def generate_status_list(status_file,z,mode,code):
    data=[]
    for M in Ms:
        for q in qs:
            for inc in incs:
                for wf in wfs:
                    data.append([generate_tag(M,q,z,inc,mode,wf,code),"new"])
    #open the file but not if it exists
    osfd = os.open(status_file, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
    with os.fdopen(osfd,'w') as fd:
        write_file_data(fd,data)
                             
def get_next_tag(status_file,seek_status):
    with open(status_file,"r+") as fd:
        while True:
            try :
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB )
                break
            except IOError as e:
                if e.errno != errno.EAGAIN:
                    raise
                else:
                    time.sleep(0.1)
        data=read_file_data(fd)
        vals=numpy.array(data)[:,1].tolist()
        found_status=""
        for val in vals:
            if(val.startswith(seek_status)):found_status=val
        if(len(found_status)>0):
            i=vals.index(found_status)
            tag=data[i][0]
            data[i][1]="processing"
            fd.seek(0);
            fd.truncate();
            write_file_data(fd,data)
        else: tag=None
        fcntl.flock(fd,fcntl.LOCK_UN)
    return tag,found_status

def write_status(status_file,tag,new_status, expect=""):
    with open(status_file,"r+") as fd:
        while True:
            try :
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB )
                break
            except IOError as e:
                if e.errno != errno.EAGAIN:
                    raise
                else:
                    time.sleep(0.1)
        data=read_file_data(fd)
        tags=numpy.array(data)[:,0].tolist()
        #print "tags=",tags
        if(tag in tags):
            i=tags.index(tag)
            #if(not expect==""):
            #    print "expect test: '"+expect+"' vs '"+data[i][1][:len(expect)]+"'"
            if(expect=="" or expect==data[i][1][:len(expect)]):
                data[i][1]=new_status
            fd.seek(0);
            fd.truncate();
            write_file_data(fd,data)
        else: print "tag '"+tag+"' not found"
        fcntl.flock(fd,fcntl.LOCK_UN)

def get_params_string(tag):
    p=read_tag(tag)
    mtot=float(p["M"])
    q=float(p["q"])
    d=float(cosmo.luminosity_distance(float(p["z"]))/units.Mpc)
    inc=math.pi/float(p["inc"])
    m1=mtot/(1.0+1.0/q)
    m2=mtot/(1.0+q)
    t0=0
    phi0=math.pi/3.0
    beta=math.pi/3.0
    lamb=3.0*math.pi/4.0
    pol=math.pi/3.0
    val={"m1":m1,"m2":m2,"tRef":t0,"phiRef":phi0,"distance":d,"lambda":lamb,"beta":beta,"inclination":inc,"polarization":pol}
    s=""
    for par in flare_submit.parnames:
        s=s+str(val[par])+" "
    return s
    
def get_code_flag(tag):
    p=read_tag(tag)
    if(p["code"]=='p'): return "--mcmc "
    else: return ""

def find_and_process(status_file,stat):
    #actions=["new","restart_at","needs_analysis"]
    actions=["new","restart_at"]
    if(not stat in actions):
        print "Nothing defined for action: ",stat
        sys.exit("Quitting.")
    tag,found=get_next_tag(status_file,stat)
    while(tag!=None):
        print "processing tag=",tag
        if(stat==actions[0]):#new
            #first make the submission script file
            argv=tag+" -1 "+get_code_flag(tag)+" -p "+get_params_string(tag)+" "+extra_flags
            argv=argv.split()
            subfile=flare_submit.generate(system,argv,status_file)
            print "*******  cwd=",os.getcwd();
            cmd = submit+" "+subfile
            i=subprocess.call(cmd,shell=True)
            print "******* -->",i
            if(no_wait_submit):write_status(status_file,tag,'submitted')
        elif(stat==actions[1]):
            restart_label=found[11:]
            argv=tag+" -1 "+get_code_flag(tag)+" -p "+get_params_string(tag)+" "+extra_flags
            argv=argv.split()
            argv.append("-r="+restart_label)
            subfile=flare_submit.generate(system,argv,status_file)
            print "R******* cwd=",os.getcwd();
            cmd = submit+" "+subfile
            print cmd
            subprocess.call(cmd,shell=True)
            print "R*******"
            if(no_wait_submit):write_status(status_file,tag,'restart_submitted')
        else:
            print "No action to take for stat='",str(stat),"'"
        tag,found=get_next_tag(status_file,stat)

    
#detect system
system="discover"
submit="sbatch "
no_wait_submit=True

if(platform.system()=="Darwin"):
    system="macos"
    submit="tcsh "
    no_wait_submit=False
    
if("status_file.txt" in args.name):
   statusfile=args.name
else:
   statusfile=os.getcwd()+"/"+args.name+"_status_file.txt"

if(args.g):
    generate_status_list(statusfile,args.z,args.m,args.c)

if(args.set is not None):
    write_status(statusfile,args.set[0],args.set[1],args.x)

elif(len(args.s)>0):
    find_and_process(statusfile,args.s)