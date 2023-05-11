import math
import numpy as np
from matplotlib import pyplot as plt

x,y,z =np.loadtxt('Exemplo.txt',skiprows=1,usecols=[0,1,2],unpack=True)
##z=range(len(y))
print(y)



valoresRecortados= [i for i in x if i!=4]
x2=range(len(valoresRecortados))
##
# print(x)

plt.ioff()
plt.title('Tempo ata entrar en SC')
plt.plot(x2,valoresRecortados)
plt.show()
