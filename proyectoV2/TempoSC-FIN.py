import math
import numpy as np
from matplotlib import pyplot as plt

z,y =np.loadtxt('Exemplo.txt',skiprows=1,usecols=[1,2],unpack=True)
y=range(len(z))
print(y)
##
# print(x)

plt.ioff()
plt.title('Tempo dende saída de SC a fin de proceso')
plt.plot(z,y)
plt.show()

