import math
import numpy as np
from matplotlib import pyplot as plt

z=np.loadtxt('Exemplo.txt',skiprows=1,usecols=[2],unpack=True)
y=range(len(z))
print(z)
##
# print(x)

plt.ioff()
plt.title('Microsegundos en entrar en SC')
plt.plot(y,z)
plt.show()

