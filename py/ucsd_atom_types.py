# -*- coding: utf-8 -*-
import random
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties

x = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
y = [1543, 17025, 37191,  21620, 9792, 4148, 1204, 214, 42, 2]

matplotlib.rc('font', family='SimHei', weight='bold')
plt.rcParams['axes.unicode_minus'] = False

#绘图。
fig, ax = plt.subplots()
b = ax.barh(range(len(x)), y, color='#6699CC')

#为横向水平的柱图右侧添加数据标签。
for rect in b:
    w = rect.get_width()
    ax.text(w, rect.get_y()+rect.get_height()/2, '%d' %
            int(w), ha='left', va='center')

#设置Y轴纵坐标上的刻度线标签。
ax.set_yticks(range(len(x)))
ax.set_yticklabels(x)

# #不要X横坐标上的label标签。
plt.xticks(([5000, 10000, 15000, 20000, 25000, 30000, 35000, 40000]))

# plt.show()


plt.ylabel('atomic types')
plt.xlabel('the number')

plt.show()
