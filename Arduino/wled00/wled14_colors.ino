/*
 * Color conversion methods
 */

void colorFromUint32(uint32_t in)
{
  col[3] = in >> 24 & 0xFF;
  col[0] = in >> 16 & 0xFF;
  col[1] = in >> 8  & 0xFF;
  col[2] = in       & 0xFF;
}

void colorHStoRGB(uint16_t hue, byte sat, byte* rgb) //hue, sat to rgb
{
  float h = ((float)hue)/65535.0;
  float s = ((float)sat)/255.0;
  byte i = floor(h*6);
  float f = h * 6-i;
  float p = 255 * (1-s);
  float q = 255 * (1-f*s);
  float t = 255 * (1-(1-f)*s);
  switch (i%6) {
    case 0: rgb[0]=255,rgb[1]=t,rgb[2]=p;break;
    case 1: rgb[0]=q,rgb[1]=255,rgb[2]=p;break;
    case 2: rgb[0]=p,rgb[1]=255,rgb[2]=t;break;
    case 3: rgb[0]=p,rgb[1]=q,rgb[2]=255;break;
    case 4: rgb[0]=t,rgb[1]=p,rgb[2]=255;break;
    case 5: rgb[0]=255,rgb[1]=p,rgb[2]=q;
  }
}

#ifndef WLED_DISABLE_HUESYNC
void colorCTtoRGB(uint16_t mired, byte* rgb) //white spectrum to rgb
{
  //this is only an approximation using WS2812B with gamma correction enabled
  if (mired > 475) {
    rgb[0]=255;rgb[1]=199;rgb[2]=92;//500
  } else if (mired > 425) {
    rgb[0]=255;rgb[1]=213;rgb[2]=118;//450
  } else if (mired > 375) {
    rgb[0]=255;rgb[1]=216;rgb[2]=118;//400
  } else if (mired > 325) {
    rgb[0]=255;rgb[1]=234;rgb[2]=140;//350
  } else if (mired > 275) {
    rgb[0]=255;rgb[1]=243;rgb[2]=160;//300
  } else if (mired > 225) {
    rgb[0]=250;rgb[1]=255;rgb[2]=188;//250
  } else if (mired > 175) {
    rgb[0]=247;rgb[1]=255;rgb[2]=215;//200
  } else {
    rgb[0]=237;rgb[1]=255;rgb[2]=239;//150
  }
}

void colorXYtoRGB(float x, float y, byte* rgb) //coordinates to rgb (https://www.developers.meethue.com/documentation/color-conversions-rgb-xy)
{
  float z = 1.0f - x - y;
  float X = (1.0f / y) * x;
  float Z = (1.0f / y) * z;
  float r = (int)255*(X * 1.656492f - 0.354851f - Z * 0.255038f);
  float g = (int)255*(-X * 0.707196f + 1.655397f + Z * 0.036152f);
  float b = (int)255*(X * 0.051713f - 0.121364f + Z * 1.011530f);
  if (r > b && r > g && r > 1.0f) {
    // red is too big
    g = g / r;
    b = b / r;
    r = 1.0f;
  } else if (g > b && g > r && g > 1.0f) {
    // green is too big
    r = r / g;
    b = b / g;
    g = 1.0f;
  } else if (b > r && b > g && b > 1.0f) {
    // blue is too big
    r = r / b;
    g = g / b;
    b = 1.0f;
  }
  // Apply gamma correction
  r = r <= 0.0031308f ? 12.92f * r : (1.0f + 0.055f) * pow(r, (1.0f / 2.4f)) - 0.055f;
  g = g <= 0.0031308f ? 12.92f * g : (1.0f + 0.055f) * pow(g, (1.0f / 2.4f)) - 0.055f;
  b = b <= 0.0031308f ? 12.92f * b : (1.0f + 0.055f) * pow(b, (1.0f / 2.4f)) - 0.055f;

  if (r > b && r > g) {
    // red is biggest
    if (r > 1.0f) {
      g = g / r;
      b = b / r;
      r = 1.0f;
    }
  } else if (g > b && g > r) {
    // green is biggest
    if (g > 1.0f) {
      r = r / g;
      b = b / g;
      g = 1.0f;
    }
  } else if (b > r && b > g) {
    // blue is biggest
    if (b > 1.0f) {
      r = r / b;
      g = g / b;
      b = 1.0f;
    }
  }
  rgb[0] = 255.0*r;
  rgb[1] = 255.0*g;
  rgb[2] = 255.0*b;
}

void colorRGBtoXY(byte* rgb, float* xy) //rgb to coordinates (https://www.developers.meethue.com/documentation/color-conversions-rgb-xy)
{
  float X = rgb[0] * 0.664511f + rgb[1] * 0.154324f + rgb[2] * 0.162028f;
  float Y = rgb[0] * 0.283881f + rgb[1] * 0.668433f + rgb[2] * 0.047685f;
  float Z = rgb[0] * 0.000088f + rgb[1] * 0.072310f + rgb[2] * 0.986039f;
  xy[0] = X / (X + Y + Z);
  xy[1] = Y / (X + Y + Z);
}
#endif

void colorFromDecOrHexString(byte* rgb, char* in)
{
  if (in[0] == 0) return;
  char first = in[0];
  uint32_t c = 0;
  
  if (first == '#' || first == 'h' || first == 'H') //is HEX encoded
  {
    c = strtoul(in +1, NULL, 16);
  } else
  {
    c = strtoul(in, NULL, 10);
  }

  rgb[3] = (c >> 24) & 0xFF;
  rgb[0] = (c >> 16) & 0xFF;
  rgb[1] = (c >>  8) & 0xFF;
  rgb[2] =  c        & 0xFF;
}

float minf (float v, float w)
{
  if (w > v) return v;
  return w;
}

float maxf (float v, float w)
{
  if (w > v) return w;
  return v;
}

void colorRGBtoRGBW(byte* rgb) //rgb to rgbw (http://codewelt.com/rgbw)
{
  float low = minf(rgb[0],minf(rgb[1],rgb[2]));
  float high = maxf(rgb[0],maxf(rgb[1],rgb[2]));
  if (high < 0.1f) return;
  float sat = 255.0f * ((high - low) / high);
  rgb[3] = (byte)((255.0f - sat) / 255.0f * (rgb[0] + rgb[1] + rgb[2]) / 3);
}


void convertHue(uint8_t light)
{
  double      hh, p, q, t, ff, s, v;
  long        i;

  s = lights[light].sat / 255.0;
  v = lights[light].bri / 255.0;

  if (s <= 0.0) {      // < is bogus, just shuts up warnings
    lights[light].colors[0] = v;
    lights[light].colors[1] = v;
    lights[light].colors[2] = v;
    return;
  }
  hh = lights[light].hue;
  if (hh >= 65535.0) hh = 0.0;
  hh /= 11850, 0;
  i = (long)hh;
  ff = hh - i;
  p = v * (1.0 - s);
  q = v * (1.0 - (s * ff));
  t = v * (1.0 - (s * (1.0 - ff)));

  switch (i) {
    case 0:
      lights[light].colors[0] = v * 255.0; colS[0] = v * 255.0;
      lights[light].colors[1] = t * 255.0; colS[1] = t * 255.0;
      lights[light].colors[2] = p * 255.0; colS[2] = p * 255.0;
      break;
    case 1:
      lights[light].colors[0] = q * 255.0; colS[0] = q * 255.0;
      lights[light].colors[1] = v * 255.0; colS[1] = v * 255.0;
      lights[light].colors[2] = p * 255.0; colS[2] = p * 255.0;
      break;
    case 2:
      lights[light].colors[0] = p * 255.0; colS[0] = p * 255.0;
      lights[light].colors[1] = v * 255.0; colS[1] = v * 255.0;
      lights[light].colors[2] = t * 255.0; colS[2] = t * 255.0;
      break;

    case 3:
      lights[light].colors[0] = p * 255.0; colS[0] = p * 255.0;
      lights[light].colors[1] = q * 255.0; colS[1] = q * 255.0;
      lights[light].colors[2] = v * 255.0; colS[2] = v * 255.0;
      break;
    case 4:
      lights[light].colors[0] = t * 255.0; colS[0] = t * 255.0;
      lights[light].colors[1] = p * 255.0; colS[1] = p * 255.0;
      lights[light].colors[2] = v * 255.0; colS[2] = v * 255.0;
      break;
    case 5:
    default:
      lights[light].colors[0] = v * 255.0; colS[0] = v * 255.0;
      lights[light].colors[1] = p * 255.0; colS[1] = p * 255.0;
      lights[light].colors[2] = q * 255.0; colS[2] = q * 255.0;
      break;
  }

}

void convertXy(uint8_t light)
{
  int optimal_bri = lights[light].bri;
  if (optimal_bri < 5) {
    optimal_bri = 5;
  }
  float Y = lights[light].y;
  float X = lights[light].x;
  float Z = 1.0f - lights[light].x - lights[light].y;

  // sRGB D65 conversion
  float r =  X * 3.2406f - Y * 1.5372f - Z * 0.4986f;
  float g = -X * 0.9689f + Y * 1.8758f + Z * 0.0415f;
  float b =  X * 0.0557f - Y * 0.2040f + Z * 1.0570f;


  // Apply gamma correction
  r = r <= 0.04045f ? r / 12.92f : pow((r + 0.055f) / (1.0f + 0.055f), 2.4f);
  g = g <= 0.04045f ? g / 12.92f : pow((g + 0.055f) / (1.0f + 0.055f), 2.4f);
  b = b <= 0.04045f ? b / 12.92f : pow((b + 0.055f) / (1.0f + 0.055f), 2.4f);

  if (r > b && r > g) {
    // red is biggest
    if (r > 1.0f) {
      g = g / r;
      b = b / r;
      r = 1.0f;
    }
  }
  else if (g > b && g > r) {
    // green is biggest
    if (g > 1.0f) {
      r = r / g;
      b = b / g;
      g = 1.0f;
    }
  }
  else if (b > r && b > g) {
    // blue is biggest
    if (b > 1.0f) {
      r = r / b;
      g = g / b;
      b = 1.0f;
    }
  }

  r = r < 0 ? 0 : r;
  g = g < 0 ? 0 : g;
  b = b < 0 ? 0 : b;

  lights[light].colors[0] = (int) (r * optimal_bri); lights[light].colors[1] = (int) (g * optimal_bri); lights[light].colors[2] = (int) (b * optimal_bri);
  colS[0] = lights[light].colors[0]; colS[1] = lights[light].colors[1]; colS[2] = lights[light].colors[2];
}

void convertCt(uint8_t light) {
  int hectemp = 10000 / lights[light].ct;
  int r, g, b;
  if (hectemp <= 66) {
    r = 255;
    g = 99.4708025861 * log(hectemp) - 161.1195681661;
    b = hectemp <= 19 ? 0 : (138.5177312231 * log(hectemp - 10) - 305.0447927307);
  } else {
    r = 329.698727446 * pow(hectemp - 60, -0.1332047592);
    g = 288.1221695283 * pow(hectemp - 60, -0.0755148492);
    b = 255;
  }
  r = r > 255 ? 255 : r;
  g = g > 255 ? 255 : g;
  b = b > 255 ? 255 : b;
  lights[light].colors[0] = r * (lights[light].bri / 255.0f); lights[light].colors[1] = g * (lights[light].bri / 255.0f); lights[light].colors[2] = b * (lights[light].bri / 255.0f);
  colS[0] = lights[light].colors[0]; colS[1] = lights[light].colors[1]; colS[2] = lights[light].colors[2];
}

RgbColor blending(float left[3], float right[3], uint8_t pixel) {
  uint8_t result[3];
  for (uint8_t i = 0; i < 3; i++) {
    float percent = (float) pixel / (float) (transitionLeds + 1);
    result[i] = (left[i] * (1.0f - percent) + right[i] * percent) / 2;
  }
  return RgbColor((uint8_t)result[0], (uint8_t)result[1], (uint8_t)result[2]);
}

RgbColor convInt(float color[3]) {
  return RgbColor((uint8_t)color[0], (uint8_t)color[1], (uint8_t)color[2]);
}

RgbColor convFloat(float color[3]) {
  return RgbColor((uint8_t)color[0], (uint8_t)color[1], (uint8_t)color[2]);
}

RgbColor blendingEntert(float left[3], float right[3], float pixel) {
  uint8_t result[3];
  for (uint8_t i = 0; i < 3; i++) {
    float percent = (float) pixel / (float) (transitionLeds + 1);
    result[i] = (left[i] * (1.0f - percent) + right[i] * percent) / 2;
  }
  return RgbColor((uint8_t)result[0], (uint8_t)result[1], (uint8_t)result[2]);
}
