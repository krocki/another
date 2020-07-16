#version 410

in vec2 uv;

uniform vec2 cursor;
uniform sampler2D tx;
uniform float palette[16*3];

out vec4 frag_color;

vec3 get_color(int k) {

  return vec3(palette[3*k], palette[3*k+1], palette[3*k+2]);
  //switch (k) {
  //  case  0: return vec3(0,     0,     0.00f); break;
  //  case  1: return vec3(1,     1,     0.67f); break;
  //  case  2: return vec3(1,     1    , 0.53f); break;
  //  case  3: return vec3(1,     0.93f, 0.47f); break;
  //  case  4: return vec3(1,     0.93f, 0.33f); break;
  //  case  5: return vec3(0.93f, 0.87f, 0.27f); break;
  //  case  6: return vec3(0.87f, 0.67f, 0.20f); break;
  //  case  7: return vec3(0.80f, 0.67f, 0.13f); break;
  //  case  8: return vec3(0.73f, 0.60f, 0.13f); break;
  //  case  9: return vec3(0.67f, 0.53f, 0.07f); break;
  //  case 10: return vec3(0.60f, 0.47f, 0.00f); break;
  //  case 11: return vec3(0.53f, 0.40f, 0.00f); break;
  //  case 12: return vec3(0.47f, 0.33f, 0.00f); break;
  //  case 13: return vec3(0.40f, 0.27f, 0.00f); break;
  //  case 14: return vec3(0.33f, 0.20f, 0.00f); break;
  //  case 15: return vec3(0.47f, 0.53f, 0.47f); break;
  //  default:
  //    return vec3(0.5f, 0.5f, 0.5f); break;
  //}
}

vec4 colormap(float v) {

  //vec3 c = colormap_a + colormap_b * cos(6.28318 * (colormap_c * vec3(v, v, v) + colormap_d));

  //return vec4(c, 1.0f);
  // gray
  vec3 c = get_color(int(v*255.0f));
  return vec4(c, 1.0f);
}

vec4 val(float v) {
  return vec4(v, v, v, 1.0f)+0.1f;
}

float dist(vec2 a, vec2 b) {
  vec2 r = b-a;
  return sqrt(r.x*r.x + r.y*r.y);
}

void main () {

  frag_color = colormap(texture(tx, uv).x);
  //frag_color += dist(uv, cursor) < 0.01f ? vec4(0, 0.5, 0, 0) : vec4(0,0,0,0);

}
