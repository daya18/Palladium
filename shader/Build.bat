glslangValidator.exe -V source/shader.glsl.vert -o build/shader.spv.vert 
glslangValidator.exe -V source/shader.glsl.frag -o build/shader.spv.frag

glslangValidator.exe -V source/GUIShader.glsl.vert -o build/GUIShader.spv.vert
glslangValidator.exe -V source/GUIShader.glsl.frag -o build/GUIShader.spv.frag

glslangValidator.exe -V source/TextShader.glsl.vert -o build/TextShader.spv.vert
glslangValidator.exe -V source/TextShader.glsl.frag -o build/TextShader.spv.frag