<div id="logo">
  <img src="https://github.com/pochishchalov/LaminateSketch/blob/main/icons/app_icon.png?raw=true" width="100"/>
</div>

# Laminate Sketch.

Приложение для работы с данными полученными из NX Fibersim и последующим размещением на чертеж

<div id="mainwindow">
  <img src="https://github.com/pochishchalov/LaminateSketch/blob/main/pictures/main_window.png?raw=true" width="800"/>
</div>

- Принимает на вход данные в форматах DFX и DWG;
- Обрабатывает полученные данные, выстраивает слои по порядку, выводит полученный "скетч" на экран;
- Сохраняет файл в формате DXF.

*Для сборки проекта необходимо:*    
*- поддержка стандарта не ниже С++20.*    
*- Qt версии не ниже 6.1.*    
*- Boost*    
*- iconv*    

! - В `CMakeList.txt` необходимо указать путь к `iconv` (Например: `C:/Dev/msys64/mingw64`).
>>>>>>> f4ad6af3f16c64b2515199bf4f1c588c042e6332
