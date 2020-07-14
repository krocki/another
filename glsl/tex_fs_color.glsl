#version 410

in vec2 uv;

uniform sampler2D tx;

out vec4 frag_color;

vec3 get_color(int k) {

  switch (k) {
    case  0: return vec3(0,     0,     0.00f); break;
    case  1: return vec3(0,     0,     0.67f); break;
    case  2: return vec3(0,     0.67f, 0.00f); break;
    case  3: return vec3(0,     0.67f, 0.67f); break;
    case  4: return vec3(0.67f, 0.00f, 0.00f); break;
    case  5: return vec3(0.67f, 0.00f, 0.67f); break;
    case  6: return vec3(0.67f, 0.33f, 0.00f); break;
    case  7: return vec3(0.67f, 0.67f, 0.67f); break;
    case  8: return vec3(0.33f, 0.33f, 0.33f); break;
    case  9: return vec3(0.33f, 0.33f, 1.00f); break;
    case 10: return vec3(0.33f, 1.00f, 0.33f); break;
    case 11: return vec3(0.33f, 1.00f, 1.00f); break;
    case 12: return vec3(1.00f, 0.33f, 0.33f); break;
    case 13: return vec3(1.00f, 0.33f, 1.00f); break;
    case 14: return vec3(1.00f, 1.00f, 0.33f); break;
    case 15: return vec3(1.00f, 1.00f, 1.00f); break;
    default:
      return vec3(0.5f, 0.5f, 0.5f); break;
  }
}

vec4 colormap(float b) {

  vec3 o = get_color(int(b*16));
  return vec4(o, 1.0f);

}

void main () {

  frag_color = colormap(texture(tx, uv).x);

}
