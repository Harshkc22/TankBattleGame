// TankBase.h - Base class for all tanks
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TankBase.generated.h"

UCLASS()
class TANKBATTLE_API ATankBase : public APawn
{
    GENERATED_BODY()

public:
    ATankBase();

protected:
    virtual void BeginPlay() override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* TankBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* TankTurret;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* TankBarrel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* ProjectileSpawnPoint;

    // Tank Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Properties")
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Properties")
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Properties")
    float MoveSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Properties")
    float TurnRate = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tank Properties")
    float TurretRotationSpeed = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireRate = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<class AProjectile> ProjectileClass;

    // Combat
    virtual void Fire();
    void RotateTurretTowards(FVector TargetLocation);
    
    // Damage System
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
                            class AController* EventInstigator, AActor* DamageCauser) override;
    
    virtual void HandleDestruction();

private:
    float LastFireTime = 0.0f;
    bool bIsDestroyed = false;

public:
    virtual void Tick(float DeltaTime) override;
    bool IsDestroyed() const { return bIsDestroyed; }
};

// TankBase.cpp
#include "TankBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Projectile.h"
#include "Kismet/GameplayStatics.h"

ATankBase::ATankBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create root collision component
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(90.0f, 90.0f, 50.0f));

    // Tank body mesh
    TankBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBody"));
    TankBody->SetupAttachment(CollisionBox);

    // Tank turret mesh
    TankTurret = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankTurret"));
    TankTurret->SetupAttachment(TankBody);

    // Tank barrel mesh
    TankBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankBarrel"));
    TankBarrel->SetupAttachment(TankTurret);

    // Projectile spawn point
    ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
    ProjectileSpawnPoint->SetupAttachment(TankBarrel);
}

void ATankBase::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

void ATankBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ATankBase::Fire()
{
    if (bIsDestroyed) return;
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastFireTime < 1.0f / FireRate) return;
    
    if (ProjectileClass && ProjectileSpawnPoint)
    {
        FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
        FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();
        
        AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
            ProjectileClass, SpawnLocation, SpawnRotation);
        
        if (Projectile)
        {
            Projectile->SetOwner(this);
            LastFireTime = CurrentTime;
        }
    }
}

void ATankBase::RotateTurretTowards(FVector TargetLocation)
{
    if (!TankTurret || bIsDestroyed) return;
    
    FVector Direction = TargetLocation - TankTurret->GetComponentLocation();
    Direction.Z = 0;
    
    FRotator TargetRotation = Direction.Rotation();
    FRotator CurrentRotation = TankTurret->GetComponentRotation();
    
    FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation, TargetRotation, 
        GetWorld()->GetDeltaSeconds(), TurretRotationSpeed);
    
    TankTurret->SetWorldRotation(FRotator(0, NewRotation.Yaw, 0));
}

float ATankBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
                            AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
    
    if (CurrentHealth <= 0 && !bIsDestroyed)
    {
        HandleDestruction();
    }
    
    return ActualDamage;
}

void ATankBase::HandleDestruction()
{
    bIsDestroyed = true;
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
    
    // Spawn explosion effect
    UGameplayStatics::SpawnEmitterAtLocation(
        GetWorld(), nullptr, GetActorLocation());
}

// PlayerTank.h - Player-controlled tank
#pragma once

#include "CoreMinimal.h"
#include "TankBase.h"
#include "PlayerTank.generated.h"

UCLASS()
class TANKBATTLE_API APlayerTank : public ATankBase
{
    GENERATED_BODY()

public:
    APlayerTank();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UCameraComponent* Camera;

    // Input
    void MoveForward(float Value);
    void Turn(float Value);
    void RotateTurret(float Value);
    void FireInput();

    // Mouse control
    void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaTime) override;

private:
    class APlayerController* PlayerControllerRef;
    FVector GetMouseHitLocation();

    virtual void HandleDestruction() override;
};

// PlayerTank.cpp
#include "PlayerTank.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

APlayerTank::APlayerTank()
{
    // Spring arm for camera
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->SetRelativeRotation(FRotator(-45.0f, 0, 0));
    SpringArm->TargetArmLength = 1000.0f;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 2.0f;

    // Camera
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
}

void APlayerTank::BeginPlay()
{
    Super::BeginPlay();
    PlayerControllerRef = Cast<APlayerController>(GetController());
}

void APlayerTank::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (PlayerControllerRef && !IsDestroyed())
    {
        FVector MouseWorldLocation = GetMouseHitLocation();
        RotateTurretTowards(MouseWorldLocation);
    }
}

void APlayerTank::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    PlayerInputComponent->BindAxis("MoveForward", this, &APlayerTank::MoveForward);
    PlayerInputComponent->BindAxis("Turn", this, &APlayerTank::Turn);
    PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APlayerTank::FireInput);
}

void APlayerTank::MoveForward(float Value)
{
    if (IsDestroyed()) return;
    
    FVector DeltaLocation = FVector::ZeroVector;
    DeltaLocation.X = Value * MoveSpeed * GetWorld()->GetDeltaSeconds();
    AddActorLocalOffset(DeltaLocation, true);
}

void APlayerTank::Turn(float Value)
{
    if (IsDestroyed()) return;
    
    FRotator DeltaRotation = FRotator::ZeroRotator;
    DeltaRotation.Yaw = Value * TurnRate * GetWorld()->GetDeltaSeconds();
    AddActorLocalRotation(DeltaRotation, true);
}

void APlayerTank::FireInput()
{
    Fire();
}

FVector APlayerTank::GetMouseHitLocation()
{
    FHitResult HitResult;
    if (PlayerControllerRef)
    {
        PlayerControllerRef->GetHitResultUnderCursor(
            ECollisionChannel::ECC_Visibility, false, HitResult);
        return HitResult.Location;
    }
    return FVector::ZeroVector;
}

void APlayerTank::HandleDestruction()
{
    Super::HandleDestruction();
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}

// EnemyTank.h - AI-controlled enemy tank
#pragma once

#include "CoreMinimal.h"
#include "TankBase.h"
#include "EnemyTank.generated.h"

UENUM(BlueprintType)
enum class EAIState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Patrolling UMETA(DisplayName = "Patrolling"),
    Chasing UMETA(DisplayName = "Chasing"),
    Attacking UMETA(DisplayName = "Attacking")
};

UCLASS()
class TANKBATTLE_API AEnemyTank : public ATankBase
{
    GENERATED_BODY()

public:
    AEnemyTank();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // AI Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float DetectionRange = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float AttackRange = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    float PatrolRadius = 1000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    EAIState CurrentState = EAIState::Idle;

    // AI Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    class UAIController* AIControllerRef;

private:
    class APlayerTank* PlayerTank;
    FVector InitialLocation;
    FVector CurrentPatrolTarget;
    FTimerHandle FireTimerHandle;

    // AI Behavior
    void UpdateAIState();
    void ExecuteAIBehavior();
    
    // State behaviors
    void HandleIdleState();
    void HandlePatrollingState();
    void HandleChasingState();
    void HandleAttackingState();
    
    // Helper functions
    bool IsPlayerInRange(float Range);
    void MoveToTarget(FVector TargetLocation);
    void FireAtPlayer();
    FVector GetRandomPatrolPoint();
    
    virtual void HandleDestruction() override;
};

// EnemyTank.cpp
#include "EnemyTank.h"
#include "PlayerTank.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

AEnemyTank::AEnemyTank()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AEnemyTank::BeginPlay()
{
    Super::BeginPlay();
    
    AIControllerRef = Cast<AAIController>(GetController());
    PlayerTank = Cast<APlayerTank>(UGameplayStatics::GetPlayerPawn(this, 0));
    InitialLocation = GetActorLocation();
    CurrentPatrolTarget = GetRandomPatrolPoint();
}

void AEnemyTank::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!IsDestroyed())
    {
        UpdateAIState();
        ExecuteAIBehavior();
    }
}

void AEnemyTank::UpdateAIState()
{
    if (!PlayerTank || PlayerTank->IsDestroyed())
    {
        CurrentState = EAIState::Patrolling;
        return;
    }
    
    float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerTank->GetActorLocation());
    
    if (DistanceToPlayer <= AttackRange)
    {
        CurrentState = EAIState::Attacking;
    }
    else if (DistanceToPlayer <= DetectionRange)
    {
        CurrentState = EAIState::Chasing;
    }
    else
    {
        CurrentState = EAIState::Patrolling;
    }
}

void AEnemyTank::ExecuteAIBehavior()
{
    switch (CurrentState)
    {
        case EAIState::Idle:
            HandleIdleState();
            break;
        case EAIState::Patrolling:
            HandlePatrollingState();
            break;
        case EAIState::Chasing:
            HandleChasingState();
            break;
        case EAIState::Attacking:
            HandleAttackingState();
            break;
    }
}

void AEnemyTank::HandleIdleState()
{
    // Tank is idle, waiting for player
}

void AEnemyTank::HandlePatrollingState()
{
    if (FVector::Dist(GetActorLocation(), CurrentPatrolTarget) < 100.0f)
    {
        CurrentPatrolTarget = GetRandomPatrolPoint();
    }
    
    MoveToTarget(CurrentPatrolTarget);
}

void AEnemyTank::HandleChasingState()
{
    if (PlayerTank && !PlayerTank->IsDestroyed())
    {
        MoveToTarget(PlayerTank->GetActorLocation());
        RotateTurretTowards(PlayerTank->GetActorLocation());
    }
}

void AEnemyTank::HandleAttackingState()
{
    if (PlayerTank && !PlayerTank->IsDestroyed())
    {
        // Stop moving and attack
        if (AIControllerRef)
        {
            AIControllerRef->StopMovement();
        }
        
        RotateTurretTowards(PlayerTank->GetActorLocation());
        
        // Check if we have line of sight
        FHitResult HitResult;
        FVector Start = GetActorLocation();
        FVector End = PlayerTank->GetActorLocation();
        
        GetWorld()->LineTraceSingleByChannel(
            HitResult, Start, End, ECollisionChannel::ECC_Visibility);
        
        if (HitResult.GetActor() == PlayerTank)
        {
            Fire();
        }
    }
}

bool AEnemyTank::IsPlayerInRange(float Range)
{
    if (!PlayerTank || PlayerTank->IsDestroyed()) return false;
    
    return FVector::Dist(GetActorLocation(), PlayerTank->GetActorLocation()) <= Range;
}

void AEnemyTank::MoveToTarget(FVector TargetLocation)
{
    if (AIControllerRef)
    {
        FAIMoveRequest MoveRequest;
        MoveRequest.SetGoalLocation(TargetLocation);
        MoveRequest.SetAcceptanceRadius(50.0f);
        
        AIControllerRef->MoveTo(MoveRequest);
    }
}

FVector AEnemyTank::GetRandomPatrolPoint()
{
    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSystem)
    {
        FNavLocation RandomLocation;
        if (NavSystem->GetRandomReachablePointInRadius(
            InitialLocation, PatrolRadius, RandomLocation))
        {
            return RandomLocation.Location;
        }
    }
    return InitialLocation;
}

void AEnemyTank::HandleDestruction()
{
    Super::HandleDestruction();
    GetWorldTimerManager().ClearTimer(FireTimerHandle);
    Destroy();
}

// Projectile.h - Projectile class
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class TANKBATTLE_API AProjectile : public AActor
{
    GENERATED_BODY()
    
public:    
    AProjectile();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* ProjectileMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UParticleSystemComponent* TrailParticles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float Damage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float LifeSpan = 3.0f;

private:
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, 
              UPrimitiveComponent* OtherComp, FVector NormalImpulse, 
              const FHitResult& Hit);

public:    
    virtual void Tick(float DeltaTime) override;
};

// Projectile.cpp
#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // Collision component
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(10.0f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    // Projectile mesh
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    ProjectileMesh->SetupAttachment(CollisionSphere);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Movement component
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 2000.0f;
    ProjectileMovement->MaxSpeed = 2000.0f;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    // Trail particles
    TrailParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticles"));
    TrailParticles->SetupAttachment(RootComponent);
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();
    
    CollisionSphere->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
    SetLifeSpan(LifeSpan);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, 
                        UPrimitiveComponent* OtherComp, FVector NormalImpulse, 
                        const FHitResult& Hit)
{
    AActor* MyOwner = GetOwner();
    if (!MyOwner) return;
    
    if (OtherActor && OtherActor != this && OtherActor != MyOwner)
    {
        UGameplayStatics::ApplyPointDamage(
            OtherActor, Damage, GetActorLocation(), 
            Hit, nullptr, this, UDamageType::StaticClass());
        
        // Spawn explosion effect
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), nullptr, GetActorLocation());
        
        // Play explosion sound
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(), nullptr, GetActorLocation());
        
        Destroy();
    }
}

void AProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// Obstacle.h - Destructible obstacles
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Obstacle.generated.h"

UENUM(BlueprintType)
enum class EObstacleType : uint8
{
    Wall UMETA(DisplayName = "Wall"),
    Barricade UMETA(DisplayName = "Barricade"),
    Building UMETA(DisplayName = "Building"),
    Rock UMETA(DisplayName = "Rock")
};

UCLASS()
class TANKBATTLE_API AObstacle : public AActor
{
    GENERATED_BODY()
    
public:    
    AObstacle();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBoxComponent* CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* ObstacleMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    EObstacleType ObstacleType = EObstacleType::Wall;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    bool bIsDestructible = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
    float MaxHealth = 50.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
    float CurrentHealth;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
                            class AController* EventInstigator, AActor* DamageCauser) override;

    void DestroyObstacle();

public:    
    virtual void Tick(float DeltaTime) override;
};

// Obstacle.cpp
#include "Obstacle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AObstacle::AObstacle()
{
    PrimaryActorTick.bCanEverTick = false;

    // Collision box
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));

    // Obstacle mesh
    ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
    ObstacleMesh->SetupAttachment(CollisionBox);
}

void AObstacle::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

float AObstacle::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                            AController* EventInstigator, AActor* DamageCauser)
{
    if (!bIsDestructible) return 0.0f;
    
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
    
    if (CurrentHealth <= 0)
    {
        DestroyObstacle();
    }
    
    return ActualDamage;
}

void AObstacle::DestroyObstacle()
{
    // Spawn destruction effects
    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), nullptr, GetActorLocation());
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), nullptr, GetActorLocation());
    
    Destroy();
}

void AObstacle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
