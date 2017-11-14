import sys
import random

# Generates the values of the input array.
def gen_array(t):
    content = ""
    for i in range(t-1):
        content += str(random.randint(-10000,10000)) + "," + "\n"
    content += str(random.randint(-10000,10000)) + "\n"
    return content

#Generates the input array.
def gen_data(i):
    size = random.randint(1,1000)
    content = ""
    content += """varsize inputArray_"""+str(i)+"""[] = {"""+"\n"
    content += gen_array(size)
    content += """};"""+"\n"
    return content

num_tests = sys.argv[1];

data = ""+"\n"

for i in range(int(num_tests)):
    data += gen_data(i)

gf = open('datgen.dat', 'w');
gf.write(data)
gf.close()

