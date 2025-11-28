
//1
// --- Penetration logic ---
    // Angle between bullet direction and surface normal
float ImpactAngleDeg = FMath::Abs(FMath::RadiansToDegrees(
    FMath::Acos(FVector::DotProduct(GetVelocity().GetSafeNormal(),
        Hit.Normal))));

// If angle > 75° (i.e., shallow grazing) and penetration depth allows,
// let the bullet continue with reduced speed.
if (ImpactAngleDeg > 75.f && PenetrationDepth > 0.f)
{
    // Reduce speed proportionally to angle (simple model)
    float SpeedLossFactor = FMath::Clamp((ImpactAngleDeg - 75.f) / 45.f, 0.f, 1.f);
    float NewSpeed = ProjectileMovement->Velocity.Size() * (1.f - SpeedLossFactor * 0.5f);
    ProjectileMovement->Velocity = GetVelocity().GetSafeNormal() * NewSpeed;

    // Optionally reduce penetration depth (not tracked per surface here)
    // Continue moving – do not destroy.
    return;
}
//2
//.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CumulativeShell.generated.h"

UCLASS()
class YOURGAME_API ACumulativeShell : public AActor
{
    GENERATED_BODY()

public:
    ACumulativeShell();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ShellMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float RaycastDistance = 400.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float MainDamage = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float ExplosionRadius = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float CenterDamage = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float EdgeDamage = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Explosion")
    float EdgeDamageRadius = 15.0f;

    void CreateExplosion(FVector Location);
};
//.cpp

#include "CumulativeShell.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

ACumulativeShell::ACumulativeShell()
{
    PrimaryActorTick.bCanEverTick = true;

    ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
    RootComponent = ShellMesh;

    ShellMesh->OnComponentHit.AddDynamic(this, &ACumulativeShell::OnHit);
}

void ACumulativeShell::BeginPlay()
{
    Super::BeginPlay();
}

void ACumulativeShell::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ACumulativeShell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    FVector Start = Hit.ImpactPoint;
    FVector End = Start + Hit.Normal * RaycastDistance;

    FHitResult RaycastHit;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);

    // Выполняем Raycast
    if (GetWorld()->LineTraceSingleByChannel(RaycastHit, Start, End, ECC_Visibility, CollisionParams))
    {
        // Уничтожаем объект на 400 мм
        if (RaycastHit.GetActor())
        {
            RaycastHit.GetActor()->Destroy();
        }

        // Создаем взрыв
        CreateExplosion(RaycastHit.ImpactPoint);
        Destroy();  // Уничтожаем снаряд
    }
}

void ACumulativeShell::CreateExplosion(FVector Location)
{
    // Визуализация взрыва
    DrawDebugSphere(GetWorld(), Location, ExplosionRadius, 12, FColor::Red, false, 2.0f);

    // Наносим урон
    TArray<AActor*> ActorsToDamage;
    UGameplayStatics::GetAllActorsWithinRange(this, Location, ExplosionRadius, ActorsToDamage);

    for (AActor* Actor : ActorsToDamage)
    {
        if (Actor == nullptr) continue;

        // Рассчитываем расстояние от центра взрыва
        float Distance = FVector::Dist(Location, Actor->GetActorLocation());

        float Damage = (Distance <= EdgeDamageRadius) ? CenterDamage :
            (Distance <= ExplosionRadius) ? EdgeDamage : 0;

        if (Damage > 0)
        {
            UGameplayStatics::ApplyDamage(Actor, Damage, nullptr, this, nullptr);
        }
    }
}