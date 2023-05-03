import math
import numpy as np
from matplotlib import pyplot as plt

x, y =np.loadtxt('Exemplo.txt',skiprows=1,usecols=[0,1],unpack=True)
print(x,y)

plt.ioff()
plt.title('Grafica')
plt.plot(x,y)
plt.show()

