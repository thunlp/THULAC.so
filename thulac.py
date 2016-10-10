#!/usr/bin/env python
# -*- coding: utf-8 -*-

from ctypes import *
import os.path

_lib = None

def init(model_path='', user_dict_path='', pre_alloc_size=1024*1024*16, t2s=False, just_seg=False):
    global _lib
    if _lib == None:
        path = os.path.dirname(os.path.realpath(__file__)) #设置so文件的位置
        _lib = cdll.LoadLibrary(path+'/libthulac.so') #读取so文件
        if len(model_path) == 0:
            model_path = path+'/models' #获取models文件夹位置
    return _lib.init(c_char_p(model_path), c_char_p(user_dict_path), pre_alloc_size, int(t2s), int(just_seg)) #调用接口进行初始化
     
def clear():
    if _lib != None: _lib.deinit()
    
def seg(data):
    assert _lib != None
    r = _lib.seg(c_char_p(data))
    assert r > 0
    p = _lib.getResult()
    s = cast(p,c_char_p)
    d = '%s'%s.value
    _lib.freeResult();
    return d.split(' ')
    
if __name__ == '__main__':
    init() #模型等文件预读取和初始化
    d = '我爱北京天安门' #待分词的句子
    print ' '.join(seg(d)) #输出分词结果
    clear() # 释放内存
