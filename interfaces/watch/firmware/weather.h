// Weather (A5), shared by the pet scenes: rain and snow falling, splashes, a lightning bolt with one short flash, leaves
// in the wind. pet::wx says which (the phone sends it, #weather); the scenes grey their palettes with pet::dim().
// Included inside a scene's namespace (like game.h), so each scene has its own copy: no include guard. Before including
// it the scene defines N, GROUND, T, rnd(), clampf(), px(), art() with 'C' = a leaf color, fb and the palette entries
// DROP, FLAKE, FLASH, STAR, SUN, plus WIND_DIR (+1: the wind blows to the right, -1: to the left), and declares:
//   float canopy(float x)  rain stops on this row at x (an umbrella, a leaf, a shield); a big number = nothing there
//   float landRow()        a random row where a drop meets the ground
//   float drift()          sideways px/s the drops get from the scene moving (a trip)
//   bool airy(uint8_t c)   sky, cloud and star colors: what a flash lights up and a bolt crosses
//   void thunder()         the pet's reaction to a bolt

bool wet() { return pet::wx == pet::RAIN || pet::wx == pet::STORM; }
bool snowing() { return pet::wx == pet::SNOW; }
int dropCount() { return pet::wx == pet::STORM ? 48 : pet::wx == pet::RAIN ? 26 : snowing() ? 34 : 0; }
float gust = 0;  // wind: 0..1, gusts come and go

const char *const LEAF_ART[2][3] = {{"..CCK", ".CCC.", "CCC.."}, {"CCC..", ".CCC.", "..CCK"}};  // tumbling in the wind

const int ND = 48;
struct {
  float x, y, v, land;
} drops[ND];
struct {
  float x, y, t;
} splash[12];
struct {
  float x, y, ph;
} leaves[5];
int nsplash = 0;
float snowCover = 0, boltT = 3, boltAge = -1, boltX = 40;  // ground white 0..1; s to the next bolt, since the last

void dropNew(int i, bool top) {
  auto &d = drops[i];
  d.x = -12 + (N + 24) * rnd(), d.y = top ? -2 - 8 * rnd() : -2 + GROUND * rnd();
  d.v = snowing() ? 9 + 7 * rnd() : 105 + 40 * rnd();
  d.land = landRow();
}

float leafX() { return WIND_DIR > 0 ? -6 - 30 * rnd() : N + 6 + 30 * rnd(); }  // upwind, off screen

void weatherStep(float dt) {
  float slant = pet::wx == pet::STORM ? 30 : 10, run = drift();  // px/s sideways (and the trip's wind)
  for (int i = 0, n = dropCount(); i < n; i++) {
    auto &d = drops[i];
    if (d.v == 0 || (snowing() != (d.v < 50))) dropNew(i, false);  // first use, or rain <-> snow
    d.y += d.v * dt, d.x += (snowing() ? 4 * sinf(T * 1.3f + i) : slant) * dt + run * dt;
    float stop = fminf(d.land, canopy(d.x));
    if (d.y < stop) continue;
    if (!snowing()) splash[nsplash] = {d.x, stop, 0}, nsplash = (nsplash + 1) % 12;
    dropNew(i, true);
  }
  for (auto &s : splash) s.t += dt;
  snowCover = clampf(snowCover + (snowing() ? dt / 40 : -dt / 90), 0, 1);  // settles in 40 s, melts in 90 s

  if (pet::wx == pet::STORM && (boltT -= dt) <= 0) {  // a bolt and one short flash every 4-10 s (photosensitivity: never a strobe)
    boltT = 4 + 6 * rnd(), boltAge = 0, boltX = 18 + 44 * rnd();
    thunder();
  }
  if (boltAge >= 0 && (boltAge += dt) > 0.4f) boltAge = -1;

  gust = pet::wx == pet::WIND ? fmaxf(0, sinf(T * 0.8f)) : 0;
  for (int i = 0; i < 5; i++) {
    auto &l = leaves[i];
    if (pet::wx != pet::WIND) {
      l.x = WIND_DIR > 0 ? -20 - 40 * rnd() : N + 20 + 40 * rnd(), l.y = 28 + (GROUND - 30) * rnd();  // under the bubble band
      continue;
    }
    l.x += WIND_DIR * (30 + 70 * gust + 10 * i) * dt, l.ph += (4 + i) * dt;
    l.y += 9 * sinf(l.ph * 0.7f) * dt;
    if (WIND_DIR > 0 ? l.x > N + 6 : l.x < -6) l.x = leafX(), l.y = 28 + (GROUND - 30) * rnd();
  }
}

void drawFlash() {  // lightning: the sky lights up for 0.08 s, the bolt stays a little longer
  if (boltAge < 0) return;
  if (boltAge < 0.08f)
    for (uint8_t &c : fb)
      if (airy(c)) c = FLASH;
  if (boltAge > 0.3f) return;
  float x = boltX;
  for (int k = 0; k < 7; k++) {
    const int SEG = 5;
    float nx = x + SEG * (h01(int(boltX) * 13 + k) - 0.5f) * 2;
    for (int y = k * SEG; y < k * SEG + SEG; y++) {
      int bx = lroundf(x + (nx - x) * (y - k * SEG) / SEG);
      if ((unsigned)bx < N && (airy(fb[y * N + bx]) || fb[y * N + bx] == FLASH)) px(bx, y, STAR);
    }
    x = nx;
  }
}

void drawDrops() {  // behind the pet: rain streaks and splashes, or snowflakes
  for (int i = 0, n = dropCount(); i < n; i++) {
    const auto &d = drops[i];
    int x = lroundf(d.x), y = lroundf(d.y);
    if (snowing()) {
      px(x, y, FLAKE);
      if (i % 4 == 0) px(x - 1, y, FLAKE), px(x + 1, y, FLAKE), px(x, y - 1, FLAKE), px(x, y + 1, FLAKE);
      continue;
    }
    float sl = pet::wx == pet::STORM ? 0.25f : 0.1f;
    for (int k = 0, n = pet::wx == pet::STORM ? 4 : 3; k < n; k++) px(lroundf(d.x - sl * k), y - k, DROP);
  }
  for (const auto &s : splash)
    if (s.t < 0.12f) px(s.x - 1, s.y - 1, DROP), px(s.x + 1, s.y - 1, DROP);
}

void drawLeaves() {  // in front of everything
  for (int i = 0; i < 5; i++)
    if (leaves[i].x > -6 && leaves[i].x < N + 6) art(LEAF_ART[sinf(leaves[i].ph) > 0], 3, lroundf(leaves[i].x), lroundf(leaves[i].y), WIND_DIR);
}

void rays(float mx, float my) {  // clear sky: slowly turning sun rays, over the sky only
  for (int k = 0; k < 8; k++) {
    float a = k * 0.785f + T * 0.4f, r = 6 + (k & 1);
    int x = mx + r * cosf(a), y = my + r * sinf(a);
    if ((unsigned)x < N && (unsigned)y < N && airy(fb[y * N + x])) px(x, y, SUN);
  }
}
