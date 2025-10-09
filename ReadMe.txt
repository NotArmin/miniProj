Image Processing program
-----------------------------------------------------------------------------------------------
Designed to run on a RISC-V DE10-Lite FPGA from a Windows 10+ host PC with WSL and Ubuntu 24.04
-----------------------------------------------------------------------------------------------
Requirements:
--------------
- WSL enabled and Ubuntu installed (use "sudo apt update" and "sudo apt upgrade" in Ubuntu terminal)

- Requires the official RISC-V GNU Toolchain: git clone https://github.com/riscv/riscv-gnu-toolchain
Install dependencies and install the compiler and add the path to your profile

- Requires the Altera USB-Blaster: 
sudo nano /etc/udev/rules.d/92-usbblaster.rules
Type this into the file:
# USB-Blaster
SUBSYSTEM=="usb", ATTRS{idVendor}=="09fb", ATTRS{idProduct}=="6001", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="09fb", ATTRS{idProduct}=="6002", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="09fb", ATTRS{idProduct}=="6003", MODE="0666"
# USB-Blaster II
SUBSYSTEM=="usb", ATTRS{idVendor}=="09fb", ATTRS{idProduct}=="6010", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="09fb", ATTRS{idProduct}=="6810", MODE="0666"
Save and restart your system

- Requires JTAG software: https://www.intel.com/content/www/us/en/software-kit/795187/intel-quartus-prime-lite-edition-design-software-version-23-1-for-linux.html
In "Additional Files" download "Intel® Quartus® Prime Programmer and Tools 23.1std.0.991"
Add the JTAG-Daemon to your path variable 
-----------------------------------------------------------------------------------------------
How to start the program:
1. Connect your DE10-Lite FPGA to your PC via USB and connect your monitor via VGA
2. Start PowerShell as admin, Win+X -> Windows PowerShell (admin)
3. "usbipd.exe list" (find the Altera USB-Blaster bus id, usually 1-1, 2-1 or 3-1)
4. "usbipd.exe bind --busid BUS_ID_HERE"
5. Start WSL Ubuntu
6. In Powershell; "usbipd.exe attach --wsl --busid BUS_ID_HERE"
7. In Ubuntu; "lsusb" should show Altera Blaster 
8. Start the JTAG-Daemon using "jtagd --user-start"
9. Use "jtagconfig" to check if the DE10-Lite board is connected successfully
9a. If "jtagconfig" does not load properly, use "killall jtagd" to break the connection and repeat from step 8
10. Enter the project environment and run "make" to compile the program
11. Run "dtekv-run main.bin" to start the program. After a couple of seconds you should see the UI on your monitor
12. Press the upper button "KEY0" to stop the program. 
13. If you want to start the program again, repeat from step 11.
13a. Note:
        The DE10-Lite often loses connection between program runs because it is quite bad.
        Use "jtagconfig" to check connection and repeat steps 8-9a as necessary.

-----------------------------------------------------------------------------------------------
How to use the program:
When starting the program, the user will be immediately shown the main menu on the User Interface.
Use the button "KEY1" to move the pointer down one step (it loops around). 
Use the right most switch "SW0" to enter a selected menu option, it works on positive edge trigger, meaning
you must manually "reload" it between enter operations.

The upload menu option will show the user 3 sample images, choose one to be processed.
After choosing one of the images, the user will be taken back to the main menu.
The user may view the chosen image by choosing the "Download / View" option, 
which will display the image on screen until another "enter" input is made.
From the main menu, navigate to the Process option to proceed in the image-process program.

The process menu will be shown to the user with all available filters as options.
The user may choose any option they want and the processed image will be shown on screen until
another "enter" input is made.
The user may apply whatever filter, however many times they want.
When the user is satisfied, choosing Return takes them to the main menu.
From here, if the user is dissatisfied with the processed image,
they may go to the upload menu and choose any of the images again, effectively removing any previous processes.

If the user wants to save their processed image, the user may choose the "Download / View" option to save.
The user will once again be shown their processed image until next "enter" operation is made.
Note, doing this will overwrite the originally chosen image.

From here, the user may choose a new image to upload to the program and repeat previous steps for their new image.
The user can access their first processed image after having downloaded it at any time through the upload menu.

Quitting the program by pressing KEY0 will restore all images to their original state and reset all processes.

-----------------------------------------------------------------------------------------------
How to run performance checks:
To run performance checks, enter the "Makefile" and add "-DRUN-PERFORMANCE-TESTS" at the end of CFLAGS
Also, navigate to the function "apply_process_and_show" in "ui_state.c". There are two versions of the function,
the above version is for performance checks and below is without performance checks. Comment out whichever 
version you do not want. 
Also, the program normally runs on -O3 compiler optimization. To change the compiler optimization, navigate to the
Makefile and at CFLAGS, change the "-O3" to "-O0" or "-O2" or whichever level of optimization you want.
NOTE: to compile without any optimization (-O0), also remove the "-fno-builtin" from CFLAGS