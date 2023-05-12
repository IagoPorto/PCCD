import math
import numpy as np
from matplotlib import pyplot as plt

##z =np.loadtxt('Exemplo.txt',skiprows=1,usecols=[4],unpack=True)
x=[0.499905018,0.3332927,0.249962568,0.199972164,0.166641504,0.142836819,0.124982206,0.11108186,0.099974227,0.09088879,0.083313935,0.076907163,0.071413279,0.066653456,0.062487178,0.071406991,0.071411326,0.07141003,0.071408592]

y=range(len(x))
print(x)
##
# print(x)

plt.ioff()
plt.title('Factor de utilización')
plt.plot(y,x)
plt.show()

