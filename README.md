# Collection-of-weapon-mechanics

## 1 Ricochet
* this code creates a normal and checks the angle of entry of the bullet into the object, if the angle of entry exceeds 75 degrees from the normal, then the bullet ricochets.

## 2 HEAT projectile
* Свойства:
** ShellMesh: компонент статической сетки, представляющий визуальное представление снаряда.
** RaycastDistance, MainDamage, ExplosionRadius, CenterDamage, EdgeDamage, EdgeDamageRadius: параметры, определяющие влияние снаряда и взрыва.
* Методы:
** OnHit: вызывается при столкновении снаряда с другим объектом. Выполняет raycast, чтобы уничтожить объект на расстоянии 400 мм и создает взрыв в месте удара.
** CreateExplosion: визуализирует взрыв и наносит урон всем актерам в определенной области, рассчитывая урон в зависимости от расстояния до центра взрыва.
