protocol
========

Network protocol library for C++11

Hi, I'm Glenn Fiedler. I'm a professional game network programmer.

When I'm coding at home I want to be able to quickly explore new game networking ideas. Also, I need a network library to use as a basis for GDC talks that I will do in the future.

This library enables that. It is based on source and ideas from my "Networking for Game Programmers" article series: http://gafferongames.com/networking-for-game-programmers/

This code is not production ready and never will be. Many things I do here are focused around quick iteration time for myself so I can explore new ideas, rather than efficiency and the best practices that I would take when developing professional level code at work.

Some things I do here that would never do at work include:

a) Extensive use of C++11 features, lambdas, shared_ptr etc
b) Usage of stl containers
c) Usage of std::string
d) Dynamic heap allocations at runtime
e) Use of exceptions to report error conditions

Please note that at the same time as wishing to explore network ideas at home, I am also using this project to explore C++11 features. So expect a bit of craziness as I learn this stuff.

cheers
