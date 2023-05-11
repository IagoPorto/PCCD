import math
import numpy as np
from matplotlib import pyplot as plt

x,y,z =np.loadtxt('Exemplo.txt',skiprows=1,usecols=[0,1,2],unpack=True)
##z=range(len(y))
print(y)

for i in x:
    if z[i]==4:
        x.pop(i)
##
# print(x)

plt.ioff()
plt.title('Tempo ata entrar en SC')
plt.plot(x,y)
plt.show()

