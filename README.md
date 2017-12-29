# GA-Firefox-
通过遗传算法使用100个半透明三角形绘制Firefox图标。（需要OpenCV环境）

## 实现方法：

**参考[科学松鼠会 遗传算法：内存中的进化](http://songshuhui.net/archives/10462)的思路，下面是我的实现方法：**

### 创建初始群体

 1. 种群内扇贝个数设定为16。
 2. 每个扇贝壳上有100个半透明三角形。每个三角形的三个点与颜色随机生成。
 3. 使用OpenCV绘制该扇贝，与理想图片逐像素对比，计算出该扇贝适应度（适应度=像素点个数*通道数/所有像素点4通道差值累积和）。

### 交叉

 1. 随机寻找两扇贝配对。
 2. 100个三角形内随机寻找n个三角形，两扇贝进行交叉操作。

### 变异

 1. 某个扇贝随机一个三角形的三点与颜色变异。

### 更新适应度

### 选择

 1. 淘汰适应度最低的两个扇贝。
 2. 为保持种群数量不变，其余扇贝内随机寻找两扇贝交叉补缺位。

### 运行参数

 1. 交叉率：0.86
 2. 变异率：0.1
 3. 迭代次数：180000
 4. 种群大小：16
 5. 三角形个数：100

### 微调

 1. 选择算子：轮盘赌算法（容易早熟），淘汰最低两个交叉补缺位（目前所用的方法）。
 2. 变异算子：变异两个三角形，变异一个三角形（两方法差别不大）。
 3. 变异率：0.2（过高，种群适应度增长缓慢），0.1（目前所用的值）.
 4. 理想图像分辨率：128*128（渲染速度过慢），32*32（目前所用的值）。

## 实验效果
Firefox理想图标：
![Firefox理想图标](http://img.blog.csdn.net/20171227221434530?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvVHVhbno3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

遗传算法180000代最优扇贝：
![这里写图片描述](http://img.blog.csdn.net/20171229142258813?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvVHVhbno3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

遗传算法历代最优扇贝：
![这里写图片描述](http://img.blog.csdn.net/20171229142358295?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvVHVhbno3/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

比较看出：实验结果形成了基本的轮廓，但是细节不好，并且迭代次数过多，与[科学松鼠会 遗传算法：内存中的进化](http://songshuhui.net/archives/10462)文章中贴出的算法结果相差很多，应该是我有很多细节、优化方面没考虑到。有兴趣的同学可以看看，如果各位有更好的思路或者优化方法的，希望能多多指导，感谢各位！

**最近找到几篇新的文章，代码还在优化，持续更新中......**
