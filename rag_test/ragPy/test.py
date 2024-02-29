'''
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
'''

import json
import matplotlib.pyplot as plt
import numpy as np
# import torch
# torch.cuda.empty_cache()
# file_path = './multifieldqa_zh.jsonl'
# question = []
# answer = []

# with open(file_path, 'r') as file:
#     for line in file:
#         data = json.loads(line)
#         question.append(data['input'])
#         answer.append(data['answers'][0])

# qa = {'question': question, 'answer': answer}
# with open('qa.json', 'w', encoding='utf-8') as f:
#     json.dump(qa, f, ensure_ascii=False, indent=4)

# print(f'Context saved to {txt_path}')

with open('min_v_acc_0.json', 'r') as file:
    data = json.load(file)
#     print(data.keys())
    # question.append(data['input'])
    # answer.append(data['answers'][0])

# print(data['10']['flat'])

x = []
for i in range(10, 150, 5):
    x.append(i)
x = np.array(x)

y_flat, y_ivf_flat, y_pq, y_ivf_pq = [], [], [], []
for i in data.keys():
    y_flat.append(data[str(i)]['flat'])
    y_ivf_flat.append(data[str(i)]['ivf_flat'])
    y_pq.append(data[str(i)]['pq'])
    y_ivf_pq.append(data[str(i)]['ivf_pq'])

y_flat = np.array(y_flat)
y_ivf_flat = np.array(y_ivf_flat)
y_pq = np.array(y_pq)
y_ivf_pq = np.array(y_ivf_pq)

plt.plot(x, y_flat, label='flat', color='red', linestyle='-')
plt.plot(x, y_ivf_flat, label='ivf_flat', color='blue', linestyle='-')
plt.plot(x, y_pq, label='pq', color='green', linestyle='-')
plt.plot(x, y_ivf_pq, label='ivf_pq', color='black', linestyle='-')

plt.xlabel('min threshold')
plt.ylabel('search accurate')

plt.legend(loc = 1)

# #plt.show()
plt.savefig('plot.png')
