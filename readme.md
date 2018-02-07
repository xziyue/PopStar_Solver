# PopStar Solver
* Actually this is not just a 'solver' for PopStar games, it is a framework of the entire game concept, including several useful game state estiamte functions and a fully playable OpenGL implementation. If you are not familiar with PopStar, I suppose you can know more by downloading and playing it yourself. The Google Play link to one of the many PopStar games is right below:
(https://play.google.com/store/apps/details?id=com.stevenjou.popstar)

* This code is written on Windows. For succuessful compilation, please put the headers of the corresponding libraries in the ./Library/ folder, and put the static libraries in the ./lib/ folder. This project relys on FreeType, GLFW, GLFW and Eigen. For it to run on other platform, one might need to adapt the Visual Studio project, and change some detailed stuff such as the filename of the font.

* There are a couple of variables controling the depth of searching. I also imposed time limit on the searching process. One can easily understand the logic of depth control and time control by analyzing GetNaiveSearchWidth() and GetNaiveBestSolution().

* Again, like most of my codes, this 'solver' is just purely a work of excitement. I wrote these things pretty quickly, so both the code and the comment suck. Read and use the code at your own risk.