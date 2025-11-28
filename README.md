# Collection-of-weapon-mechanics

## 1 Ricochet
* this code creates a normal and checks the angle of entry of the bullet into the object, if the angle of entry exceeds 75 degrees from the normal, then the bullet ricochets.

## 2 HEAT projectile

* Properties:

 ShellMesh: a component of a static grid representing a visual representation of a projectile.

 RaycastDistance, MainDamage, ExplosionRadius, CenterDamage, EdgeDamage, EdgeDamageRadius: parameters that determine the impact of the projectile and explosion.

* Methods:

 OnHit: called when a projectile collides with another object. Performs a raycast to destroy an object at a distance of 400 mm and creates an explosion at the point of impact.

 CreateExplosion: visualizes an explosion and deals damage to all actors in a certain area, calculating damage depending on the distance to the center of the explosion.
