"""Character spritesheets for StrixVerse.

Six sheets, one per palette zone, so a character keeps its per-player colours:
the sheet is a white/grey mask and the renderer multiplies it by the zone's
palette colour. Grey pixels shade for free - 255 is the flat palette colour,
200 is a shadow, 255+ is not possible so highlights are done by leaving the
lighter areas at full and darkening everything else.

Frames are 24x48, which is exactly the size the player is drawn at
(kPlayerWidth = 32 * 0.75, kPlayerHeight = 32 * 1.5), so at zoom 1 one texel
is one screen pixel and nothing is resampled.

Layout is one horizontal strip per zone:

    frames 0-3    Idle    a slow breath
    frames 4-9    Walk    a six-frame stride
    frames 10-11  Jump    launch, then airborne

The figure faces right. The renderer flips it by swapping the U coordinates,
so there is no mirrored art to keep in step.
"""

import io
import os
from PIL import Image

W, H = 24, 48
IDLE, WALK, JUMP = 4, 6, 2
FRAMES = IDLE + WALK + JUMP

ZONES = ("skin", "hair", "eyes", "shirt", "trousers", "boots")

OUT = "C:/Projects/StrixVerse/Client/assets/character"

# Shading levels, as a multiplier on the palette colour.
LIT = 255      # flat palette colour
MID = 214      # the shaded side of a limb
DARK = 176     # underside, boot sole, hair depth


class Canvas:
    """One frame's worth of pixels per zone."""

    def __init__(self):
        self.zones = {z: {} for z in ZONES}

    def rect(self, zone, x, y, w, h, level=LIT):
        for py in range(int(y), int(y + h)):
            for px in range(int(x), int(x + w)):
                if 0 <= px < W and 0 <= py < H:
                    self.zones[zone][(px, py)] = level

    def px(self, zone, x, y, level=LIT):
        if 0 <= x < W and 0 <= y < H:
            self.zones[zone][(int(x), int(y))] = level


def build_frame(pose):
    """One frame from a pose dictionary.

    The pose carries only offsets - everything structural is here, so the
    figure cannot drift between animations.

    The figure is drawn facing the viewer rather than in profile: at 24x48 a
    profile head is four pixels of face and reads as a smudge, and the renderer
    mirrors the whole sprite for facing anyway, which a profile would make
    obvious and a front view does not.
    """
    c = Canvas()

    bob = pose["bob"]              # whole upper body, in pixels
    arm_f = pose["arm_front"]      # right arm swing, +down
    arm_b = pose["arm_back"]       # left arm swing
    leg_f = pose["leg_front"]      # right leg, +forward
    leg_b = pose["leg_back"]
    knee_f = pose["knee_front"]    # how far the leg is drawn up
    knee_b = pose["knee_back"]
    lean = pose["lean"]            # torso lean, +right

    # ---- head ------------------------------------------------------------
    # 12 wide, 14 tall, centred. Built symmetrically: an asymmetric head at
    # this size reads as damage rather than as character.
    hx, hy = 6 + lean, 2 + bob

    c.rect("skin", hx + 1, hy + 4, 10, 9)          # face
    c.rect("skin", hx + 2, hy + 13, 8, 1, MID)     # jaw, shaded across
    c.rect("skin", hx + 4, hy + 14, 4, 2, MID)     # neck

    # Hair: a cap over the crown that comes down both temples equally, with a
    # fringe one pixel lower across the brow.
    c.rect("hair", hx + 1, hy, 10, 4)
    c.rect("hair", hx, hy + 1, 1, 6, MID)          # left temple
    c.rect("hair", hx + 11, hy + 1, 1, 6, MID)     # right temple
    c.rect("hair", hx + 1, hy + 4, 10, 1)          # fringe
    c.px("hair", hx, hy, MID)
    c.px("hair", hx + 11, hy, MID)

    # ---- eyes ------------------------------------------------------------
    # Two 2x2 blocks, evenly inset. Anything larger became sunglasses.
    c.rect("eyes", hx + 3, hy + 7, 2, 2)
    c.rect("eyes", hx + 7, hy + 7, 2, 2)

    # ---- torso -----------------------------------------------------------
    tx, ty = 5 + lean, 16 + bob
    c.rect("shirt", tx + 1, ty, 12, 13)
    c.rect("shirt", tx + 1, ty + 13, 12, 2, MID)   # hem shadow
    c.rect("shirt", tx + 1, ty, 1, 15, MID)        # both sides shaded, evenly
    c.rect("shirt", tx + 12, ty, 1, 15, MID)

    # ---- arms ------------------------------------------------------------
    # Sleeve then hand, one on each side. The near arm is unshaded and the far
    # arm is a step darker so the two read as depth rather than as one wide
    # body.
    c.rect("shirt", tx - 2, ty + 1 + arm_b, 3, 8, DARK)
    c.rect("skin", tx - 2, ty + 9 + arm_b, 3, 3, MID)

    c.rect("shirt", tx + 13, ty + 1 + arm_f, 3, 8)
    c.rect("skin", tx + 13, ty + 9 + arm_f, 3, 3)

    # ---- hips and legs ---------------------------------------------------
    px_, py_ = 6 + lean, 31 + bob
    c.rect("trousers", px_, py_, 12, 4)
    c.rect("trousers", px_, py_ + 3, 12, 1, MID)

    def leg(dx, knee, level):
        lx = px_ + 1 + dx
        top = py_ + 4
        length = max(8 - knee, 2)
        c.rect("trousers", lx, top, 4, length, level)
        return lx, top + length

    # Far leg first, so the near leg overlaps it.
    bx, bfoot = leg(0 + leg_b, knee_b, DARK)
    fx, ffoot = leg(6 + leg_f, knee_f, LIT)

    # ---- boots -----------------------------------------------------------
    # A 5-wide boot with the toe on the leading edge and a darker sole, so a
    # foot reads as a foot and not as the end of a trouser leg.
    def boot(x, y, level, sole):
        c.rect("boots", x - 1, y, 5, 3, level)
        c.rect("boots", x - 1, y + 3, 5, 1, sole)
        c.px("boots", x + 4, y + 2, level)         # toe

    boot(bx, bfoot, DARK, DARK)
    boot(fx, ffoot, LIT, MID)

    return c


def poses():
    """Twelve poses: four idle, six walk, two jump."""
    out = []

    # Idle: a breath. One pixel of rise at the top of the cycle, arms hanging.
    for i, bob in enumerate((0, 0, -1, 0)):
        out.append(dict(bob=bob, arm_front=0, arm_back=0,
                        leg_front=0, leg_back=0,
                        knee_front=0, knee_back=0, lean=0))

    # Walk: a six-frame stride. The legs scissor, the arms counter-swing, and
    # the body dips on the two contact frames so the step lands rather than
    # gliding.
    stride = ((3, -3, 0, 2, -1, 1, 0),
              (2, -2, 1, 1, 0, 1, 0),
              (0, 0, 2, 0, -1, 0, 0),
              (-3, 3, 2, 0, 1, -1, 0),
              (-2, 2, 1, 0, 0, -1, 0),
              (0, 0, 0, 2, -1, 0, 0))
    for lf, lb, kf, kb, arm_f, arm_b, bob in stride:
        out.append(dict(bob=bob, arm_front=arm_f, arm_back=arm_b,
                        leg_front=lf, leg_back=lb,
                        knee_front=kf, knee_back=kb, lean=1))

    # Jump: the launch is a crouch with the arms coming up; the airborne frame
    # tucks both legs and throws the arms out.
    out.append(dict(bob=1, arm_front=-2, arm_back=-1,
                    leg_front=1, leg_back=-1,
                    knee_front=3, knee_back=3, lean=0))
    out.append(dict(bob=-1, arm_front=-3, arm_back=-2,
                    leg_front=2, leg_back=-2,
                    knee_front=4, knee_back=2, lean=1))

    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    frames = [build_frame(p) for p in poses()]
    assert len(frames) == FRAMES, len(frames)

    for zone in ZONES:
        sheet = Image.new("RGBA", (W * FRAMES, H), (0, 0, 0, 0))
        px = sheet.load()

        for index, frame in enumerate(frames):
            ox = index * W
            for (x, y), level in frame.zones[zone].items():
                px[ox + x, y] = (level, level, level, 255)

        path = os.path.join(OUT, "%s.png" % zone)
        sheet.save(path)
        print("%-14s %dx%d  %d frames" % (zone, sheet.width, sheet.height, FRAMES))


if __name__ == "__main__":
    main()
