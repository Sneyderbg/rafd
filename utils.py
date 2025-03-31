import math
from typing import Union

from pyray import Vector2, vector2_rotate


# mostly written by gemini
class Vec2:
    def __init__(
        self,
        x: float = 0,
        y: float = 0,
        vec: Union["Vec2", "Vector2", None] = None,
    ):
        if vec:
            self.v = Vector2(vec.x, vec.y)
        else:
            self.v = Vector2(x, y)

    @property
    def x(self):
        return self.v.x

    @x.setter
    def x(self, value):
        self.v.x = value

    @property
    def y(self):
        return self.v.y

    @y.setter
    def y(self, value):
        self.v.y = value

    def __add__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x + other.x, self.y + other.y)
        else:
            return Vec2(self.x + other, self.y + other)

    def __sub__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x - other.x, self.y - other.y)
        else:
            return Vec2(self.x - other, self.y - other)

    def __mul__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x * other.x, self.y * other.y)
        else:
            return Vec2(self.x * other, self.y * other)

    def __radd__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x + other.x, self.y + other.y)
        else:
            return Vec2(self.x + other, self.y + other)

    def __rsub__(self, other):
        if isinstance(other, Vec2):
            return Vec2(other.x - self.x, other.y - self.y)
        else:
            return Vec2(other.x - self, other.y - self)

    def __rmul__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x * other.x, self.y * other.y)
        else:
            return Vec2(self.x * other, self.y * other)

    def __truediv__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x / other.x, self.y / other.y)
        else:
            return Vec2(self.x / other, self.y / other)

    def __floordiv__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x // other.x, self.y // other.y)
        else:
            return Vec2(self.x // other, self.y // other)

    def __mod__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x % other.x, self.y % other.y)
        else:
            return Vec2(self.x % other, self.y % other)

    def __pow__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x**other.x, self.y**other.y)
        else:
            return Vec2(self.x**other, self.y**other)

    def __gt__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x > other.x, self.y > other.y)
        else:
            return Vec2(self.x > other, self.y > other)

    def __le__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x <= other.x, self.y <= other.y)
        else:
            return Vec2(self.x <= other, self.y <= other)

    def __ge__(self, other):
        if isinstance(other, Vec2):
            return Vec2(self.x >= other.x, self.y >= other.y)
        else:
            return Vec2(self.x >= other, self.y >= other)

    def __eq__(self, other):
        if isinstance(other, Vec2):
            return self.x == other.x and self.y == other.y
        else:
            return self.x == other and self.y == other

    def __ne__(self, other):
        if isinstance(other, Vec2):
            return self.x != other.x or self.y != other.y
        else:
            return self.x != other or self.y != other

    def __isub__(self, other):
        if isinstance(other, Vec2):
            self.x -= other.x
            self.y -= other.y
        else:
            self.x -= other
            self.y -= other
        return self

    def __iadd__(self, other):
        if isinstance(other, Vec2):
            self.x += other.x
            self.y += other.y
        else:
            self.x += other
            self.y += other
        return self

    def __imul__(self, other):
        if isinstance(other, Vec2):
            self.x *= other.x
            self.y *= other.y
        else:
            self.x *= other
            self.y *= other
        return self

    def __idiv__(self, other):
        if isinstance(other, Vec2):
            self.x /= other.x
            self.y /= other.y
        else:
            self.x /= other
            self.y /= other
        return self

    def __ifloordiv__(self, other):
        if isinstance(other, Vec2):
            self.x //= other.x
            self.y //= other.y
        else:
            self.x //= other
            self.y //= other
        return self

    def __imod__(self, other):
        if isinstance(other, Vec2):
            self.x %= other.x
            self.y %= other.y
        else:
            self.x %= other
            self.y %= other
        return self

    def __ipow__(self, other):
        if isinstance(other, Vec2):
            self.x **= other.x
            self.y **= other.y
        else:
            self.x **= other
            self.y **= other
        return self

    def __neg__(self):
        return Vec2(-self.x, -self.y)

    def __hash__(self) -> int:
        return hash((int(self.x), int(self.y)))

    def length_sqr(self):
        return self.x * self.x + self.y * self.y

    def length(self):
        return math.sqrt(self.length_sqr())

    def normalize(self):
        magn = self.length()
        return Vec2(vec=self / magn) if magn > 0 else Vec2(vec=self)

    def rotate(self, rad):
        return Vec2(vec=vector2_rotate(self.v, rad))
