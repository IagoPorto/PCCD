import math
import numpy as np
from matplotlib import pyplot as plt

y =np.loadtxt('Exemplo.txt',skiprows=1,usecols=[1],unpack=True)
x=range(len(y))
print(y)
print(x)

plt.ioff()
plt.title('Grafica')
plt.plot(x,y)
plt.show()

