import math
import numpy as np
from matplotlib import pyplot as plt

##z=np.loadtxt('Exemplo.txt',skiprows=1,usecols=[2],unpack=True)
##y=range(len(z))
x=[0.0193,0.0252,0.0178,0.0255,88.8325]
#y=[400,400,400,400,3600]
y=[400,800,1200,1600,3600]

# print(x)

plt.ioff()
plt.title('Tiempo de cambio E-L')
plt.plot(y,x)
plt.show()

