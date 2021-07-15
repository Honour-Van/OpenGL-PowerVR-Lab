# 贪吃蛇

一条蛇（负责展现体型）

头、尾（负责表示动效）





每次：

- 如果在格点处：利用头的方向、头的位置，计算运动过程（这个需要保存）；将运动的头节点加入body，生成新的头部；销毁尾部节点，将尾指针指向body最后一个
- 如果不在格点处：头按照已有方向继续运动，尾部朝向body最后一个节点的位置移动。直到上一次预备的计算过程完成
- 如果即将到达treat，蛇加长，treat重新计算
- 如果碰到身体中任意一节，



需要保存：

- 头的方向
- 头的位置（用于生成一系列的位置）



重新封装





## v2

考虑尾部的动效的增加会需要大量位置信息，为了性能，思考删掉尾部的动效，总体上呈现出蠕动的效果。



## 一些解决方法

extern变量：直接放到一个文件里

在划分实现层级时，如果将吃到treat的逻辑判断加进来，则蛇的运动就是直接要实现的原子单元

但如果着眼于难以实现的蛇的运动，则一个方块的运动甚至都是一个单元。这说明了适当的单元划分对于代码实现的重要性。

