import csv
import os
from glob import glob

def print2DArray(array):
    for i in range(len(array)):
        for j in range(len(array[0])):
            print("{}, ".format(array[i][j]),end='')
        print()
    return

dirPath = os.path.dirname(os.path.realpath(__file__)) + '/V7/'
globPattern = dirPath + "*.out"

problemSizes = [0,8,16,32,64,128]
# problemSizes = [0,1024,2048,3072,4096]
threadCounts = [0,1,2,3,4,6,8,12,16,32,48,64]
# threadCounts = [0,1,2,3,4,5,6,7]
filePaths = glob(globPattern)

resultsTimings = [['0' for x in range(len(problemSizes))] for y in range(len(threadCounts))] 
resultsIterations = [['0' for x in range(len(problemSizes))] for y in range(len(threadCounts))] 

for i in range(len(threadCounts)):
    resultsTimings[i][0] = threadCounts[i]
    resultsIterations[i][0] = threadCounts[i]

for j in range(len(problemSizes)):
    resultsTimings[0][j] = problemSizes[j]
    resultsIterations[0][j] = problemSizes[j]

for path in filePaths:
    with open(path,'r') as f:
        data = f.read().split('\n')
        try:
            if not data[0].split()[0] == "slurmstepd:":
                threadCount = data[0].split()[-1]
                sizeOfPlane = data[1].split()[-1]
                iterations = data[2].split()[-1]
                time = data[3].split()[-1][:-1]

                i = threadCounts.index(int(threadCount))
                j = problemSizes.index(int(sizeOfPlane))
                resultsTimings[i][j] = time
                resultsIterations[i][j] = iterations
            pass
        except Exception as e:
            print(data)
            # raise e

with open('timings.csv','w') as f:
    w = csv.writer(f,delimiter=',')
    w.writerows(resultsTimings)

with open('iterations.csv','w') as f:
    w = csv.writer(f,delimiter=',')
    w.writerows(resultsIterations)