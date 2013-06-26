#!/usr/bin/env python

#Import relevant modules
import sys
import os
#Define variables
gpfile=[]
filt=[]
# Interpret command line
xopt = 1
popt = 0
i=2
while i < len(sys.argv):
    if (sys.argv[i]=='-x'):
        xopt=0
        i+=1
    elif (sys.argv[i]=='-p'):
        popt=1
        i+=1
        plotfile=sys.argv[i]
        i+=1
    else: break
# Main program
ls=os.listdir('.')
for line in ls:
    case=line.find('gnuplot_'+sys.argv[1])
    if case != -1: gpfile.append(line)
while i<len(sys.argv):
    for filenm in gpfile:
        file1=file(filenm,'r',1)
        line=(file1.readline(-1))
        casex=line.find("xlabel '"+sys.argv[i]+"'")
        if casex!=-1:
            casey=line.find("ylabel '"+sys.argv[i+1]+"'")
            if casey!=-1: filt.append(filenm)
        file1.close()
    i+=2
#Display to screen
if xopt:
    for filenm in filt: os.system('gnuplot -persist < '+filenm)
#Write to ps
if popt:
    file1=file('gnuplot_'+sys.argv[1]+'_tmp.gp','w',1)
    file1.write('set term post;\n')
    for filenm in filt:
        file2=file(filenm,'r',1)
        for line in file2: file1.write(line)
        file2.close()
    file1.close()
    os.system('gnuplot < gnuplot_'+sys.argv[1]+'_tmp.gp > '+plotfile)
# Remove files
for filenm in gpfile: os.remove(filenm)
if popt: os.remove('gnuplot_'+sys.argv[1]+'_tmp.gp')
