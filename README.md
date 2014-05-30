#Android-avc-test
>This project allows to check directly if an avc rule (i.e., scon:tcon:class:perm) is allowed by the SELinux policy.

---
#HOW TO RUN (tested on Nexus 7 2013)

1.	make the the avc_test executables
2.	run the test with the following command:	"./avc_test -i input_file [-o output_file]"

[-o output_file]:  if an output_file is not specified the results will be written on stdout.

---
#INFO
>This exeutable is compiled for armeabi-v7a, if you want to compile it for other targets change the Application.mk