
//1
// --- Penetration logic / Логика проникновения ---

// Calculate angle between bullet direction and surface normal (in degrees)
// Вычисляем угол между направлением полёта снаряда и нормалью поверхности (в градусах)
float ImpactAngleDeg = FMath::Abs(FMath::RadiansToDegrees(
    FMath::Acos(FVector::DotProduct(
        GetVelocity().GetSafeNormal(),  // Normalized bullet velocity / Нормализованная скорость снаряда
        Hit.Normal                       // Surface normal at impact point / Нормаль поверхности в точке удара
    ))));

// If impact angle > 75° (shallow/grazing hit) AND penetration is still possible,
// allow bullet to continue with reduced speed instead of stopping
// Если угол удара > 75° (касательное попадание) И ещё есть запас проникновения,
// позволяем снаряду продолжить полёт с уменьшенной скоростью вместо остановки
if (ImpactAngleDeg > 75.f && PenetrationDepth > 0.f)
{
    // Calculate speed loss factor: 0 at 75°, 1 at 120° (clamped)
    // Рассчитываем коэффициент потери скорости: 0 при 75°, 1 при 120° (с ограничением)
    float SpeedLossFactor = FMath::Clamp(
        (ImpactAngleDeg - 75.f) / 45.f,  // Normalize angle to 0-1 range / Нормализуем угол в диапазон 0-1
        0.f, 1.f
    );
    
    // Reduce speed: up to 50% loss depending on angle
    // Уменьшаем скорость: потеря до 50% в зависимости от угла
    float NewSpeed = ProjectileMovement->Velocity.Size() * (1.f - SpeedLossFactor * 0.5f);
    
    // Apply new velocity while preserving direction
    // Применяем новую скорость, сохраняя направление полёта
    ProjectileMovement->Velocity = GetVelocity().GetSafeNormal() * NewSpeed;

    // Optional: reduce remaining penetration depth here if tracking per-surface
    // Опционально: здесь можно уменьшить оставшийся запас проникновения, если ведётся учёт по поверхностям
    
    // Continue projectile movement – do not destroy actor
    // Продолжаем движение снаряда – не уничтожаем актёр
    return;
}
// If condition not met: proceed with normal hit handling / Если условие не выполнено: продолжаем обычную обработку попадания






//2
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CumulativeShell.generated.h"

/**
 * ACumulativeShell - HEAT/HE-Frag type projectile with cumulative jet and explosion
 * ACumulativeShell - Снаряд кумулятивного/осколочно-фугасного типа с кумулятивной струёй и взрывом
 */
UCLASS()
class YOURGAME_API ACumulativeShell : public AActor
{
    GENERATED_BODY()

public:
    // Constructor / Конструктор
    ACumulativeShell();

protected:
    // Called when actor spawns in the world / Вызывается при появлении актёра в мире
    virtual void BeginPlay() override;

public:
    // Called every frame / Вызывается каждый кадр
    virtual void Tick(float DeltaTime) override;

    // Dynamic function bound to mesh hit event / Динамическая функция, привязанная к событию удара меша
    UFUNCTION()
    void OnHit(
        UPrimitiveComponent* HitComp,      // Component that was hit / Компонент, по которому попал снаряд
        AActor* OtherActor,                // Actor that was hit / Актёр, по которому попал снаряд
        UPrimitiveComponent* OtherComp,    // Component of the hit actor / Компонент поражённого актёра
        FVector NormalImpulse,             // Impulse applied from collision / Импульс, применённый при столкновении
        const FHitResult& Hit              // Detailed collision data / Детальные данные о столкновении
    );

private:
    // Visual mesh component for the shell / Визуальный компонент меша для снаряда
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ShellMesh;

    // === Damage Settings / Настройки урона ===
    
    // Max distance for cumulative jet raycast (in cm) / Максимальная дальность луча кумулятивной струи (в см)
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float RaycastDistance = 400.0f;

    // Direct damage from cumulative jet hit / Прямой урон от попадания кумулятивной струи
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float MainDamage = 100.0f;

    // === Explosion Settings / Настройки взрыва ===
    
    // Radius of explosion damage falloff / Радиус затухания урона взрыва
    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float ExplosionRadius = 100.0f;

    // Maximum damage at explosion center / Максимальный урон в центре взрыва
    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float CenterDamage = 100.0f;

    // Minimum damage at explosion edge / Минимальный урон на краю взрыва
    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float EdgeDamage = 20.0f;

    // Radius around center where full damage is applied / Радиус вокруг центра, где применяется полный урон
    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float EdgeDamageRadius = 15.0f;

    // Helper function to spawn explosion VFX and apply damage / Вспомогательная функция для создания визуальных эффектов взрыва и нанесения урона
    void CreateExplosion(FVector Location);
};
//.cpp

#include "CumulativeShell.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"  // For debug visualization / Для отладочной визуализации

ACumulativeShell::ACumulativeShell()
{
    // Enable ticking every frame / Включаем обновление каждый кадр
    PrimaryActorTick.bCanEverTick = true;

    // Create and initialize static mesh component / Создаём и инициализируем компонент статического меша
    ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
    RootComponent = ShellMesh;  // Set as root for transform hierarchy / Устанавливаем как корневой для иерархии трансформаций

    // Bind hit event to OnHit function / Привязываем событие удара к функции OnHit
    ShellMesh->OnComponentHit.AddDynamic(this, &ACumulativeShell::OnHit);
}

void ACumulativeShell::BeginPlay()
{
    Super::BeginPlay();
    // Additional initialization can go here / Дополнительная инициализация может быть добавлена здесь
}

void ACumulativeShell::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Per-frame logic can go here / Логика каждый кадр может быть добавлена здесь
}

void ACumulativeShell::OnHit(
    UPrimitiveComponent* HitComp, 
    AActor* OtherActor, 
    UPrimitiveComponent* OtherComp, 
    FVector NormalImpulse, 
    const FHitResult& Hit)
{
    // Start raycast from impact point / Начинаем рейкаст из точки удара
    FVector Start = Hit.ImpactPoint;
    
    // End point: extend along surface normal by RaycastDistance / Конечная точка: продолжаем вдоль нормали поверхности на RaycastDistance
    FVector End = Start + Hit.Normal * RaycastDistance;

    FHitResult RaycastHit;  // Store raycast hit result / Храним результат рейкаста
    FCollisionQueryParams CollisionParams;  // Configure query parameters / Настраиваем параметры запроса
    CollisionParams.AddIgnoredActor(this);  // Ignore self to prevent false hits / Игнорируем себя, чтобы избежать ложных срабатываний

    // Perform line trace (raycast) in Visibility channel / Выполняем трассировку луча в канале видимости
    if (GetWorld()->LineTraceSingleByChannel(RaycastHit, Start, End, ECC_Visibility, CollisionParams))
    {
        // If raycast hit an actor, destroy it (cumulative jet effect)
        // Если луч попал в актёр — уничтожаем его (эффект кумулятивной струи)
        if (RaycastHit.GetActor())
        {
            RaycastHit.GetActor()->Destroy();
        }

        // Spawn explosion effects and apply area damage at impact point
        // Создаём эффекты взрыва и наносим урон по области в точке попадания
        CreateExplosion(RaycastHit.ImpactPoint);
        
        // Destroy the projectile actor after explosion / Уничтожаем актёр снаряда после взрыва
        Destroy();
    }
    // Note: If raycast misses, projectile is not destroyed – consider adding timeout/fuse logic
    // Примечание: Если луч не попал, снаряд не уничтожается — рассмотрите добавление логики таймера/взрывателя
}

void ACumulativeShell::CreateExplosion(FVector Location)
{
    // === Visual Effects / Визуальные эффекты ===
    
    // Draw debug sphere to visualize explosion radius (editor/dev builds only)
    // Рисуем отладочную сферу для визуализации радиуса взрыва (только в редакторе/дебаг-сборках)
    DrawDebugSphere(
        GetWorld(), 
        Location,           // Explosion center / Центр взрыва
        ExplosionRadius,    // Sphere radius / Радиус сферы
        12,                 // Segments for sphere mesh / Сегменты для сетки сферы
        FColor::Red,        // Debug color / Цвет отладки
        false,              // Persistent? false = auto-destroy / Сохранять? false = автоудаление
        2.0f                // Duration in seconds / Длительность в секундах
    );

    // === Damage Application / Применение урона ===
    
    // Collect all actors within explosion radius / Собираем все актёры в радиусе взрыва
    TArray<AActor*> ActorsToDamage;
    UGameplayStatics::GetAllActorsWithinRange(
        this, 
        Location, 
        ExplosionRadius, 
        ActorsToDamage
    );

    // Iterate through affected actors / Перебираем поражённые актёры
    for (AActor* Actor : ActorsToDamage)
    {
        if (Actor == nullptr) continue;  // Safety check / Проверка на null

        // Calculate distance from explosion center to actor / Вычисляем расстояние от центра взрыва до актёра
        float Distance = FVector::Dist(Location, Actor->GetActorLocation());

        // Damage falloff calculation:
        // - Full damage within EdgeDamageRadius
        // - Reduced damage between EdgeDamageRadius and ExplosionRadius
        // - No damage beyond ExplosionRadius
        // Расчёт затухания урона:
        // - Полный урон в пределах EdgeDamageRadius
        // - Сниженный урон между EdgeDamageRadius и ExplosionRadius
        // - Нет урона за пределами ExplosionRadius
        float Damage = (Distance <= EdgeDamageRadius) ? CenterDamage :
                       (Distance <= ExplosionRadius) ? EdgeDamage : 0;

        // Apply damage if calculated value is positive / Применяем урон, если рассчитанное значение положительно
        if (Damage > 0)
        {
            UGameplayStatics::ApplyDamage(
                Actor,          // Target actor / Целевой актёр
                Damage,         // Damage amount / Количество урона
                nullptr,        // Instigator controller (optional) / Контроллер инициатора (опционально)
                this,           // Damage causer / Источник урона
                nullptr         // Damage type class (optional) / Класс типа урона (опционально)
            );
        }
    }
}


// BaseHitscanWeapon.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseHitscanWeapon.generated.h"

// Enum for fire modes / Перечисление режимов огня
UENUM(BlueprintType)
enum class EFireMode : uint8
{
    Single      UMETA(DisplayName = "Single"),
    Burst       UMETA(DisplayName = "Burst"),
    Automatic   UMETA(DisplayName = "Automatic")
};

UCLASS(Abstract, Blueprintable)
class YOURGAME_API ABaseHitscanWeapon : public AActor
{
    GENERATED_BODY()

public:
    ABaseHitscanWeapon();
    virtual void Tick(float DeltaTime) override;

    // === Core Functions / Основные функции ===
    
    // Fire weapon - called by player input / Выстрел — вызывается через ввод игрока
    UFUNCTION(BlueprintCallable, Category = "Weapon|Combat")
    virtual void Fire();

    // Start/stop firing for automatic mode / Начало/остановка стрельбы для авто-режима
    UFUNCTION(BlueprintCallable, Category = "Weapon|Combat")
    virtual void StartFiring();
    
    UFUNCTION(BlueprintCallable, Category = "Weapon|Combat")
    virtual void StopFiring();

    // Reload weapon / Перезарядка оружия
    UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
    virtual void Reload();

    // === Getters / Геттеры ===
    UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
    float GetCurrentSpread() const { return CurrentSpread; }
    
    UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
    int32 GetAmmoInClip() const { return AmmoInClip; }
    
    UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
    int32 GetTotalAmmo() const { return TotalAmmo; }

protected:
    virtual void BeginPlay() override;

    // === Internal Logic / Внутренняя логика ===
    
    // Perform single shot with spread and recoil / Выполнение одного выстрела с разбросом и отдачей
    virtual void PerformShot();
    
    // Apply recoil to camera/weapon / Применение отдачи к камере/оружию
    virtual void ApplyRecoil();
    
    // Update spread based on firing state / Обновление разброса в зависимости от состояния стрельбы
    virtual void UpdateSpread(float DeltaTime);
    
    // Handle burst fire timing / Обработка тайминга очереди
    virtual void HandleBurstFire(float DeltaTime);

    // Spawn muzzle VFX and play sound / Создание визуальных эффектов дула и воспроизведение звука
    virtual void SpawnMuzzleEffects();

    // === Damage Application / Применение урона ===
    virtual void ApplyDamage(const FHitResult& Hit);

    // === Components / Компоненты ===
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* WeaponRoot;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* MuzzleSocket;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* CameraAttachment;

    // === Weapon Stats / Характеристики оружия ===
    
    // Fire mode / Режим огня
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
    EFireMode FireMode = EFireMode::Automatic;

    // Damage at point-blank range / Урон в упор
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
    float BaseDamage = 25.0f;

    // Damage falloff start distance (cm) / Дистанция начала затухания урона (см)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
    float DamageFalloffStart = 2000.0f;

    // Damage falloff end distance (cm) / Дистанция полного затухания урона (см)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
    float DamageFalloffEnd = 8000.0f;

    // === Fire Rate & Burst / Скорострельность и очередь ===
    
    // Time between shots (seconds) / Время между выстрелами (сек)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|FireRate")
    float FireRate = 0.1f;

    // Burst count (shots per burst) / Количество выстрелов в очереди
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|FireRate", meta = (EditCondition = "FireMode == EFireMode::Burst"))
    int32 BurstCount = 3;

    // Delay between burst shots / Задержка между выстрелами в очереди
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|FireRate", meta = (EditCondition = "FireMode == EFireMode::Burst"))
    float BurstDelay = 0.08f;

    // === Recoil Settings / Настройки отдачи ===
    
    // Vertical recoil per shot (degrees) / Вертикальная отдача за выстрел (градусы)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    FVector2D RecoilPerShot = FVector2D(1.5f, 0.3f);

    // Recoil recovery speed (degrees/sec) / Скорость восстановления отдачи (град/сек)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float RecoilRecoverySpeed = 8.0f;

    // Camera shake scale / Сила тряски камеры
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Recoil")
    float CameraShakeScale = 1.0f;

    // === Spread & Accuracy / Разброс и точность ===
    
    // Minimum spread (degrees) / Минимальный разброс (градусы)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Accuracy")
    float MinSpread = 0.5f;

    // Maximum spread (degrees) / Максимальный разброс (градусы)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Accuracy")
    float MaxSpread = 5.0f;

    // Spread increase per shot / Увеличение разброса за выстрел
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Accuracy")
    float SpreadPerShot = 0.8f;

    // Spread recovery rate (degrees/sec) / Скорость восстановления точности (град/сек)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Accuracy")
    float SpreadRecoveryRate = 3.0f;

    // === Ammo System / Система боеприпасов ===
    
    // Ammo per clip / Патронов в магазине
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo")
    int32 ClipSize = 30;

    // Starting total ammo / Стартовый общий запас патронов
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo")
    int32 StartingTotalAmmo = 120;

    // Reload time (seconds) / Время перезарядки (сек)
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo")
    float ReloadTime = 2.0f;

    // === VFX & SFX / Визуальные и звуковые эффекты ===
    
    // Muzzle flash particle / Частица дульной вспышки
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    UParticleSystem* MuzzleFlashTemplate;

    // Fire sound / Звук выстрела
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    USoundBase* FireSound;

    // Reload sound / Звук перезарядки
    UPROPERTY(EditDefaultsOnly, Category = "Weapon|Effects")
    USoundBase* ReloadSound;

    // === Runtime Variables / Переменные времени выполнения ===
    
    // Current spread value / Текущее значение разброса
    float CurrentSpread;
    
    // Accumulated recoil / Накопленная отдача
    FVector2D CurrentRecoil;
    
    // Ammo tracking / Отслеживание патронов
    int32 AmmoInClip;
    int32 TotalAmmo;
    
    // Fire timing / Тайминг стрельбы
    float LastFireTime;
    bool bIsFiring;
    bool bIsReloading;
    
    // Burst fire state / Состояние очереди
    int32 CurrentBurstShots;
    float LastBurstShotTime;

    // Timer handles / Дескрипторы таймеров
    FTimerHandle ReloadTimerHandle;
};
// BaseHitscanWeapon.cpp
#include "BaseHitscanWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"

ABaseHitscanWeapon::ABaseHitscanWeapon()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root component / Создаём корневой компонент
    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    RootComponent = WeaponRoot;

    // Create mesh component / Создаём компонент меша
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(WeaponRoot);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    // Create sockets / Создаём сокеты
    MuzzleSocket = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleSocket"));
    MuzzleSocket->SetupAttachment(MeshComponent);

    CameraAttachment = CreateDefaultSubobject<USceneComponent>(TEXT("CameraAttachment"));
    CameraAttachment->SetupAttachment(MeshComponent);

    // Initialize runtime variables / Инициализируем переменные
    CurrentSpread = MinSpread;
    CurrentRecoil = FVector2D::ZeroVector;
    bIsFiring = false;
    bIsReloading = false;
}

void ABaseHitscanWeapon::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize ammo / Инициализируем боеприпасы
    AmmoInClip = ClipSize;
    TotalAmmo = StartingTotalAmmo;
}

void ABaseHitscanWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Update spread recovery / Обновляем восстановление точности
    if (!bIsFiring && CurrentSpread > MinSpread)
    {
        CurrentSpread = FMath::Max(MinSpread, CurrentSpread - SpreadRecoveryRate * DeltaTime);
    }
    
    // Update recoil recovery / Обновляем восстановление отдачи
    if (!bIsFiring && !CurrentRecoil.IsZero())
    {
        CurrentRecoil = FMath::Lerp(CurrentRecoil, FVector2D::ZeroVector, RecoilRecoverySpeed * DeltaTime);
        if (CurrentRecoil.Size() < 0.01f) CurrentRecoil = FVector2D::ZeroVector;
    }
    
    // Handle burst fire timing / Обрабатываем тайминг очереди
    if (bIsFiring && FireMode == EFireMode::Burst)
    {
        HandleBurstFire(DeltaTime);
    }
}

void ABaseHitscanWeapon::StartFiring()
{
    if (bIsReloading || AmmoInClip <= 0) return;
    
    bIsFiring = true;
    
    // Fire immediately for single/burst, auto will continue in Tick
    // Стреляем сразу для одиночного/очереди, авто продолжит в Tick
    if (FireMode != EFireMode::Automatic)
    {
        Fire();
    }
}

void ABaseHitscanWeapon::StopFiring()
{
    bIsFiring = false;
    CurrentBurstShots = 0;
}

void ABaseHitscanWeapon::Fire()
{
    if (bIsReloading || AmmoInClip <= 0) 
    {
        // Play empty click sound if needed / Проиграть звук холостого щелчка при необходимости
        return; 
    }

    // Check fire rate / Проверка скорострельности
    float TimeSinceLastShot = GetWorld()->GetTimeSeconds() - LastFireTime;
    if (TimeSinceLastShot < FireRate && FireMode == EFireMode::Automatic)
    {
        return;
    }

    // Consume ammo / Расходуем патрон
    AmmoInClip--;
    LastFireTime = GetWorld()->GetTimeSeconds();

    // Perform shot logic / Выполняем логику выстрела
    PerformShot();
    
    // Apply recoil and VFX / Применяем отдачу и эффекты
    ApplyRecoil();
    SpawnMuzzleEffects();
    
    // Update spread / Обновляем разброс
    CurrentSpread = FMath::Min(MaxSpread, CurrentSpread + SpreadPerShot);
}

void ABaseHitscanWeapon::PerformShot()
{
    // Get fire direction from muzzle / Получаем направление выстрела из дула
    FVector Start = MuzzleSocket->GetComponentLocation();
    FVector Forward = MuzzleSocket->GetForwardVector();
    
    // Apply spread / Применяем разброс
    if (CurrentSpread > 0.f)
    {
        Forward = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(Forward, CurrentSpread);
    }
    
    FVector End = Start + Forward * DamageFalloffEnd;
    
    // Line trace / Трассировка луча
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.bReturnPhysicalMaterial = true;
    
    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    
    // Debug visualization / Отладочная визуализация
    #if ENABLE_DRAW_DEBUG
    DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End, 
                  FColor::Red, false, 2.0f, 0, 1.5f);
    #endif
    
    // Apply damage if hit / Применяем урон при попадании
    if (bHit)
    {
        ApplyDamage(Hit);
    }
}

void ABaseHitscanWeapon::ApplyDamage(const FHitResult& Hit)
{
    if (!Hit.GetActor()) return;
    
    // Calculate distance falloff / Рассчитываем затухание по дистанции
    float Distance = FVector::Dist(MuzzleSocket->GetComponentLocation(), Hit.ImpactPoint);
    float DamageMultiplier = 1.0f;
    
    if (Distance > DamageFalloffStart)
    {
        float FalloffT = FMath::Clamp((Distance - DamageFalloffStart) / 
                                      (DamageFalloffEnd - DamageFalloffStart), 0.f, 1.f);
        DamageMultiplier = FMath::Lerp(1.0f, 0.3f, FalloffT); // Min 30% damage at max range
    }
    
    float FinalDamage = BaseDamage * DamageMultiplier;
    
    // Apply damage via GameplayStatics / Применяем урон через GameplayStatics
    UGameplayStatics::ApplyDamage(
        Hit.GetActor(),
        FinalDamage,
        GetInstigatorController(),
        this,
        nullptr // Can add UDamageType subclass here / Можно добавить подкласс UDamageType
    );
    
    // Optional: spawn impact VFX / Опционально: создаём эффекты попадания
    // UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactVFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
}

void ABaseHitscanWeapon::ApplyRecoil()
{
    // Add recoil to accumulated value / Добавляем отдачу к накопленному значению
    CurrentRecoil += RecoilPerShot;
    CurrentRecoil.Y = FMath::Clamp(CurrentRecoil.Y, -RecoilPerShot.Y, RecoilPerShot.Y); // Limit horizontal
    
    // Apply to camera via PlayerController / Применяем к камере через PlayerController
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && CameraAttachment)
    {
        // Simple camera offset - in production use CameraShake or spring arm
        // Простое смещение камеры — в продакшене используйте CameraShake или spring arm
        FVector CameraOffset = FVector(
            -CurrentRecoil.X * 0.5f,  // Backward pull / Тяга назад
            0, 
            CurrentRecoil.Y * 0.3f     // Horizontal drift / Горизонтальный дрейф
        );
        CameraAttachment->AddLocalOffset(CameraOffset);
    }
    
    // Play camera shake / Проигрываем тряску камеры
    // PC->PlayerCameraManager->StartCameraShake(RecoilShakeClass, CameraShakeScale);
}

void ABaseHitscanWeapon::UpdateSpread(float DeltaTime)
{
    // Handled in Tick / Обрабатывается в Tick
}

void ABaseHitscanWeapon::HandleBurstFire(float DeltaTime)
{
    if (CurrentBurstShots < BurstCount)
    {
        float TimeSinceBurstShot = GetWorld()->GetTimeSeconds() - LastBurstShotTime;
        if (TimeSinceBurstShot >= BurstDelay)
        {
            Fire();
            CurrentBurstShots++;
            LastBurstShotTime = GetWorld()->GetTimeSeconds();
        }
    }
    else
    {
        StopFiring(); // End burst / Завершаем очередь
    }
}

void ABaseHitscanWeapon::SpawnMuzzleEffects()
{
    if (MuzzleFlashTemplate && MuzzleSocket)
    {
        UGameplayStatics::SpawnEmitterAttached(
            MuzzleFlashTemplate,
            MuzzleSocket,
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
    
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleSocket->GetComponentLocation());
    }
}

void ABaseHitscanWeapon::Reload()
{
    if (bIsReloading || AmmoInClip == ClipSize || TotalAmmo <= 0) return;
    
    bIsReloading = true;
    StopFiring();
    
    // Play reload sound / Проигрываем звук перезарядки
    if (ReloadSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
    }
    
    // Start reload timer / Запускаем таймер перезарядки
    GetWorldTimerManager().SetTimer(
        ReloadTimerHandle,
        this,
        &ABaseHitscanWeapon::FinishReload,
        ReloadTime,
        false
    );
    
    // Optional: play reload animation / Опционально: проигрываем анимацию перезарядки
    // OnReloadStart.Broadcast();
}

void ABaseHitscanWeapon::FinishReload()
{
    // Calculate ammo to reload / Рассчитываем количество патронов для перезарядки
    int32 AmmoNeeded = ClipSize - AmmoInClip;
    int32 AmmoToLoad = FMath::Min(AmmoNeeded, TotalAmmo);
    
    AmmoInClip += AmmoToLoad;
    TotalAmmo -= AmmoToLoad;
    
    bIsReloading = false;
    
    // Optional: broadcast reload complete / Опционально: уведомляем о завершении перезарядки
    // OnReloadComplete.Broadcast();
}


// WeaponAttachmentSystem.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponAttachmentSystem.generated.h"

// Struct for attachment stats / Структура для характеристик обвеса
USTRUCT(BlueprintType)
struct FAttachmentStats
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DamageModifier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RecoilModifier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpreadModifier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReloadSpeedModifier = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ADSModifier = 1.0f; // Aim Down Sights speed / Скорость прицеливания
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class YOURGAME_API UWeaponAttachmentSystem : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponAttachmentSystem();
    
    // Attach modification to weapon slot / Присоединение модификации к слоту оружия
    UFUNCTION(BlueprintCallable, Category = "Weapon|Attachments")
    bool AttachModification(
        FName SlotName,           // e.g., "Muzzle", "Scope", "Underbarrel"
        TSubclassOf<class UAttachmentData> AttachmentClass,
        class ABaseHitscanWeapon* TargetWeapon
    );
    
    // Remove modification from slot / Удаление модификации из слота
    UFUNCTION(BlueprintCallable, Category = "Weapon|Attachments")
    bool DetachModification(FName SlotName);
    
    // Get combined stats from all attachments / Получение суммарных характеристик всех обвесов
    UFUNCTION(BlueprintPure, Category = "Weapon|Attachments")
    FAttachmentStats GetCombinedStats() const;

protected:
    // Map of slot name -> attached modification data
    // Карта: имя слота -> данные присоединённой модификации
    UPROPERTY()
    TMap<FName, class UAttachmentData*> ActiveAttachments;

    // Apply stats to weapon / Применение характеристик к оружию
    void ApplyStatsToWeapon(ABaseHitscanWeapon* Weapon);
};


// WeaponLibrary.h - Static helper functions / Статические вспомогательные функции
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponLibrary.generated.h"

UCLASS()
class YOURGAME_API UWeaponLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Calculate damage with linear falloff / Расчёт урона с линейным затуханием
    UFUNCTION(BlueprintPure, Category = "Weapon|Utilities")
    static float CalculateFalloffDamage(
        float BaseDamage,
        float Distance,
        float FalloffStart,
        float FalloffEnd,
        float MinDamageRatio = 0.3f
    );
    
    // Generate spread vector with gaussian distribution / Генерация вектора разброса с гауссовым распределением
    UFUNCTION(BlueprintPure, Category = "Weapon|Utilities")
    static FVector ApplyGaussianSpread(FVector Direction, float SpreadDegrees);
    
    // Interpolate recoil with exponential decay / Интерполяция отдачи с экспоненциальным затуханием
    UFUNCTION(BlueprintPure, Category = "Weapon|Utilities")
    static FVector2D SmoothRecoilRecovery(
        FVector2D CurrentRecoil,
        FVector2D TargetRecoil,
        float RecoverySpeed,
        float DeltaTime
    );
};
// WeaponLibrary.cpp
#include "WeaponLibrary.h"
#include "Kismet/KismetMathLibrary.h"

float UWeaponLibrary::CalculateFalloffDamage(
    float BaseDamage,
    float Distance,
    float FalloffStart,
    float FalloffEnd,
    float MinDamageRatio)
{
    if (Distance <= FalloffStart) return BaseDamage;
    if (Distance >= FalloffEnd) return BaseDamage * MinDamageRatio;
    
    float T = (Distance - FalloffStart) / (FalloffEnd - FalloffStart);
    return FMath::Lerp(BaseDamage, BaseDamage * MinDamageRatio, T);
}

FVector UWeaponLibrary::ApplyGaussianSpread(FVector Direction, float SpreadDegrees)
{
    // Gaussian distribution gives more weight to center / Гауссово распределение даёт больший вес центру
    float U1 = FMath::FRand();
    float U2 = FMath::FRand();
    float StdDev = SpreadDegrees / 3.0f; // 99.7% within 3σ
    
    float AngleOffset = FMath::Sqrt(-2.0f * FMath::Log(1.0f - U1)) * FMath::Cos(2.0f * PI * U2) * StdDev;
    float ElevationOffset = FMath::Sqrt(-2.0f * FMath::Log(1.0f - U1)) * FMath::Sin(2.0f * PI * U2) * StdDev;
    
    FRotator OffsetRot(ElevationOffset, AngleOffset, 0);
    return OffsetRot.RotateVector(Direction).GetSafeNormal();
}

FVector2D UWeaponLibrary::SmoothRecoilRecovery(
    FVector2D CurrentRecoil,
    FVector2D TargetRecoil,
    float RecoverySpeed,
    float DeltaTime)
{
    // Exponential interpolation for natural feel / Экспоненциальная интерполяция для естественного ощущения
    float Alpha = 1.0f - FMath::Exp(-RecoverySpeed * DeltaTime);
    return FMath::Lerp(CurrentRecoil, TargetRecoil, Alpha);
}