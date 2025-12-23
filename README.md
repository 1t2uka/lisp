# :books: My lisp

本项目是我阅读和实践[build your own lisp](https://buildyourownlisp.com/)一书时，根据该书指导手动实现lisp解释器并学习c语言的记录

## :sparkles: 目标
+ []通过c语言理解便成语言解释器的基本原理和构造
+ []实现一个具备基本功能（表达式求值，数据类型，控制流）的lisp解释器
+ []掌握c语言的内存管理，数据结构和错误处理等核心功能
+ []使用cpp重构本项目，使用cpp新特性

## :gear: 环境

**语言**：c
**操作系统**：Ubuntu 24.04
**构建工具**：gcc


## :construction-site: 进度与功能实现

| 阶段 | 章节 | 核心功能 | 状态 |
| :---: | :---: | :--- | :---: |
| I | 1-6章 | 基础Repl, 输入, 逆波兰表示法, 基本数据结构 | 已完成 |
| II | 7-10章 | S-Expressions, Q-Expressions, 抽象语法树, 垃圾回收 | 进行中 |
| III | 11-13章 | 环境, 内置函数, 错误处理 | 待开始 |
| IV | 14章 | lisp函数定义, 高级特性 | 待开始 |

## :rocket: 构建和运行
本项目使用c语言编写，需要确保当前系统上安装有`gcc`和`make`
```bash
sudo apt update
sudo apt install build-essential

#编译
make
#运行
./prompt
#清除
make clean
```
## :books: Note
`grammar:`The rules that process and understand infinite number of different things with a finite number of *re-write* rules.

## :books: C
**enum**: a declaration of variables which under the hood are automatically assigned integer constant values

**pointer**:a number representing the starting index of some data in memory
+ using fixed  size to representing a large struct (containing many other sub structs)
+ representing dynamic list
+ modify the input parameters using output whihout copy any data

**Stack**: the memory where all of temporary variables and data structures as manipulate and edit when program lives

**Heap**: a section of memory put aside for storage of object whth a longer lifespan, what has to be manually allocated and deallocated(malloc & free)
