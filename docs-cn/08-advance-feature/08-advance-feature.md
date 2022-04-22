---
title: 高级功能
---

本章介绍以下TDengine中的高级功能。

## 连续查询

连续查询是一个按照预设频率自动执行的查询功能，提供按照时间窗口的聚合查询能力，是一种简化的时间驱动流式计算。
 
## 订阅

轻量级的数据订阅与推送服务。连续写入到 TDengine 中的时序数据能够被自动推送到订阅客户端。
 
## 缓存

提供写驱动的缓存管理机制，将每个表最近写入的一条记录持续保存在缓存中，可以提供高性能的最近状态查询。
 
## 用户定义函数

支持用户编码的聚合函数和标量函数，在查询中嵌入并使用用户定义函数，拓展查询的能力和功能。