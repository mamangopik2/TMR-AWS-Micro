import math
num = []
for a in range(1,101):
    num.append(a)

max_batch = 5
lastSourceIndex=0

repetition = math.ceil(len(num)/max_batch)
for rep in range(repetition):
    buffer = []
    for i in range(max_batch):
        buffer.append(num[lastSourceIndex])
        lastSourceIndex +=1
    print(buffer,len(buffer))



# print(num)