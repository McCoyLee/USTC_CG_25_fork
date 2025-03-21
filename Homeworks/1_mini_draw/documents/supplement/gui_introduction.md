# GUI 简介
本文旨在为提供图形用户界面（GUI）的基础知识，以及介绍几种常用的GUI开发工具，特别是我们将在接下来的作业中使用的 Dear ImGui 库。通过学习和使用如 Qt 和 Dear ImGui 等工具，即使是编程经验较少的同学也可以创建出功能强大且外观精美的应用程序。

## 什么是GUI ？
GUI，全称图形用户界面（Graphical User Interface），是一种用户界面，允许用户通过图形图标和视觉指示器（如按钮、菜单、图标等）与电子设备进行交互，而不仅仅是通过文本命令。这是与早期计算机界面——命令行界面（CLI）的主要区别，CLI要求用户必须记住并输入指令。

GUI的出现大大简化了计算机的操作，使得没有编程背景的普通用户也能轻松使用计算机和软件。

## GUI的优势

- **直观性**：通过图标和布局，用户可以直观地理解软件的功能，无需记忆复杂的命令。
- **易用性**：用户可以通过点击、拖拽等简单操作进行复杂任务的执行。
- **美观**：与纯文本界面相比，GUI支持各种视觉效果，可以创建更吸引人的应用。

## 常用的GUI开发工具

### Qt (本次作业不使用)

Qt 是一个跨平台的 C++ 库，广泛用于开发GUI应用程序。它不仅限于GUI编程，也支持SQL数据库操作、网络、多线程等多种功能。详细可以参考 [Qt 介绍](https://github.com/Ubpa/USTC_CG/blob/master/Homeworks/1_MiniDraw/documents/QtIntro.md)

- **跨平台性**：使用Qt开发的应用可以在多种操作系统上运行，如Windows、Mac OS、Linux等。
- **信号与槽机制**：Qt特有的事件处理机制，使得开发者可以很方便地处理用户的行为。

### Dear ImGui
[Dear ImGui](https://github.com/ocornut/imgui) 是一个为游戏和实时应用提供简单接口的图形界面库。与其他GUI库不同，Dear ImGui特别适合于快速原型开发和工具制作。

- **即时模式GUI**：ImGui采用的是一种不同于传统保留模式GUI的设计思想，开发者通过每一帧重新绘制GUI元素来管理UI的状态，这使得它在开发中非常灵活。
- **易于集成**：Dear ImGui可以很容易地集成到游戏或其他图形应用中，支持多种渲染器和平台。

Dear ImGui 有如下特点：

- **开发效率**：提供了大量预制的UI组件，如窗口、按钮、滑动条等，可以快速搭建界面。
- **轻量级**：库的设计注重效率和简洁，使其成为游戏和实时应用的理想选择。
- **灵活性**：即时模式的GUI设计让开发者能够轻松处理复杂的用户交互和动态界面变化。

我们将在接下来的几次作业中使用 Dear ImGui 实现作业的 GUI 框架。