#version 420 core
in vec4 gl_FragCoord;
 
out vec4 frag_color;

uniform int max_iterations;
uniform vec4 color_ranges;
uniform float center_x;
uniform float center_y;
uniform float zoom;
uniform float width;
uniform float height;
uniform bool mandelbrot;
uniform float constant_x;
uniform float constant_y;
 
int get_iterations()
{

    float x = ((gl_FragCoord.x / width - 0.5) * zoom + center_x) * 5.0;
    float y = ((gl_FragCoord.y / height - 0.5) * zoom + center_y) * 5.0;
 
    int iterations = 0;
    float cx = x;
    float cy = y;
 
    while (iterations < max_iterations)
    {
        float tx = x;
        if(!mandelbrot) {
            x = (x * x - y * y) + constant_x;
            y = (2.0 * tx * y) + constant_y;
        }
        else {
            x = (x * x - y * y) + cx;
            y = (2.0 * tx * y) + cy;
        }

         
        float dist = x * x + y * y;
         
        if (dist > 4.0)
        break;
 
        ++iterations;
    }
    return iterations;
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
 
vec4 return_color()
{

    int iter = get_iterations();
    if (iter == max_iterations)
    {
        gl_FragDepth = 0.0f;
        return vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }    


    float iterations = float(iter) / max_iterations;   
 

    gl_FragDepth = iterations;
    
    vec4 color_0 = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec4 color_1 = vec4(0.0f, 0.3f, 0.5f, 1.0f);
    vec4 color_2 = vec4(0.9f, 0.5f, 0.0f, 1.0f);
    vec4 color_3 = vec4(0.7f, 0.1f, 0.4f, 1.0f);
 
    float fraction = 0.0f;
    if (iterations < color_ranges[1])
    {
        fraction = (iterations - color_ranges[0]) / (color_ranges[1] - color_ranges[0]);
        return mix(color_0, color_1, fraction);
    }
    else if(iterations < color_ranges[2])
    {
        fraction = (iterations - color_ranges[1]) / (color_ranges[2] - color_ranges[1]);
        return mix(color_1, color_2, fraction);
    }
    else
    {
        fraction = (iterations - color_ranges[2]) / (color_ranges[3] - color_ranges[2]);
        return mix(color_2, color_3, fraction);
    }

}
 
void main()
{
    frag_color = return_color();
}