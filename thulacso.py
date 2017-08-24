#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os.path

from datetime import datetime
from ctypes import cdll, c_char, c_char_p, cast, POINTER
from functools import reduce
#from thulac import ThulacSo


class ThulacSo():
    def __init__(self, model_path='', user_dict_path='', pre_alloc_size=1024*1024*16, t2s=False, just_seg=False):
        self.lib = None
        if self.lib == None:
            #path = os.path.dirname(os.path.realpath(__file__)) #设置so文件的位置
            path = '/usr/local/lib/python3.5/dist-packages/thulac-0.1.1.1-py3.5.egg/thulac'
            self.lib = cdll.LoadLibrary(path+'/libthulac.so') #读取so文件
            if len(model_path) == 0:
                model_path = path+'/models' #获取models文件夹位置
        self.lib.init(c_char_p(model_path.encode('utf-8')), c_char_p(user_dict_path.encode('utf-8')), pre_alloc_size, int(t2s), int(just_seg)) #调用接口进行初始化

    def clear(self):
        if self.lib != None: self.lib.deinit()

    def cut(self, data):
        assert self.lib != None
        r = self.lib.seg(c_char_p(data.encode('utf-8')))
        assert r > 0
        self.lib.getResult.restype = POINTER(c_char)
        p = self.lib.getResult()
        s = cast(p,c_char_p)
        d = '%s'%s.value.decode('utf-8')
        self.lib.freeResult();
        array = []
        array += (reduce(lambda x, y: x + [y.split('_')], d.split(' '), []))
        return array

if __name__ == '__main__':
    thu1 = ThulacSo() #模型等文件预读取和初始化
    d = '我爱北京天安门' #待分词的句子

    print(datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
    for i in range(1,1000):
        thu1.cut(d)
        #print (thul.cut(d)) #输出分词结果
    print(datetime.now().strftime('%Y-%m-%d %H:%M:%S'))

    thu1.clear() # 释放内存