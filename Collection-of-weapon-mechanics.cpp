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