'''
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
'''

import json

def read_jsonl(file_path, txt_path):
    #context = ''
    num = 1
    with open(file_path, 'r') as file:
        for line in file:
            data = json.loads(line)
            context = data['context']

            with open(txt_path + str(num) + '.txt', 'w') as file:
                file.write(context)
            
            num +=1
            print(num)

    print(f'Context saved to {txt_path}')

def context_clean(context, newContext):
    contexts = ''
    with open(context, 'r') as file:
        context = file.readlines()
        for i in range(len(context)):
            contexts += context.replace('\n', ' ').replace('\r', ' ').replace('\t', ' ').replace('\xa0', ' ')

    with open(newContext, 'w') as file:
        file.write(contexts)

def load_qa():
    with open('qa.json', 'r') as file:
        data = json.load(file)
    
    return data

if __name__ == "__main__":
    read_jsonl('./multifieldqa_zh.jsonl', './source_context/')
