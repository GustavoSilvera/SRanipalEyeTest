// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"			   // UCameraComponent
#include "Components/SceneComponent.h"		   // USceneComponent
#include "Components/InputComponent.h"		   // InputComponent

/// WARNING: SRanipal only supports Windows development currently
// #ifdef _WIN32 
#include "SRanipalEye.h"      // SRanipal Module Framework
#include "SRanipalEye_Core.h" // SRanipal Eye Tracker
#include "SRanipalEye_Framework.h"

#include "EyeTrackerCameraPawn.generated.h"

UCLASS()
class SRANIPALEYETEST_API AEyeTrackerCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AEyeTrackerCameraPawn();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	// debugging
	void ErrMsg(const FString &message, const bool isFatal);
	APlayerController *pc;

	UWorld *World; // to get info about the world: time, frames, etc.

private:
	// Camera Variables
	// Editable from within the editor (useful when reparenting existing blueprint)
	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent *VRCameraRoot;

	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent *FirstPersonCam;

	// SRanipal variables
    SRanipalEye_Core *SRanipal;
	SRanipalEye_Framework *SRanipalFramework;
	FVector LEyeOrigin{FVector::ZeroVector};
	FVector LGazeRay{FVector::ZeroVector};
	FVector REyeOrigin{FVector::ZeroVector};
	FVector RGazeRay{FVector::ZeroVector};
};
