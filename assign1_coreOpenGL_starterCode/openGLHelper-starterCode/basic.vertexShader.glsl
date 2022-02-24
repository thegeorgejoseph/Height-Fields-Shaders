#version 150

in vec3 position;
in vec4 color,neighbour;
out vec4 col;

uniform int smoothen;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  if (smoothen == 0){

  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  col = color;
 }else{
    //replacing the middle element 'height' axis of the vec4 with the average of the four neighbours
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position[0],(neighbour[0] + neighbour[1] + neighbour[2] + neighbour[3])/4.0f,position[2], 1.0f);
    col = color;
  }
}

