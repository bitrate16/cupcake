// Demo of using files in Cupcake
// First compile interpreter with:
// > bash compile.sh install

// Second compile StreamApi & Files modules with:
// > bash buildso.sh src/modules/StreamApi.cpp bin/StreamApi.so
// > bash buildso.sh src/modules/Files.cpp bin/Files.so
// >> this will generate two files: StreamApi.so, Files.so

// Run this file with:
// ck -f res/FileDemo.ck

stdio.println('StreamApi load response: ' + NativeLoader.load('../bin/StreamApi.so'));
stdio.println('Files load response: '     + NativeLoader.load('../bin/Files.so'));

// Create new File instance
var f = File('myfile.txt');
f.createNewFile();

// Create new Printer instance & write example string
var p = Printer(f);
p.println('Hello World!');
p.close(); 	

// Reate new Scanner instance & read single line
var s = Scanner(f);
stdio.println('File content: ' + s.readLine());
s.close();