// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"	   // UCameraComponent
#include "Components/SceneComponent.h" // USceneComponent
#include "Components/InputComponent.h" // InputComponent
#include <cstdint>
#include <vector>

/// WARNING: SRanipal only supports Windows development currently
// #ifdef _WIN32
#include "SRanipalEye.h"	  // SRanipal Module Framework
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
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

	void WriteDataToFile() const;

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
	// Eye Tracker Variables
	SRanipalEye_Core *SRanipal;				  // SRanipalEye_Core.h
	SRanipalEye_Framework *SRanipalFramework; // SRanipalEye_Framework.h
	ViveSR::anipal::Eye::EyeData EyeData;	  // SRanipal_Eyes_Enums.h
	FFocusInfo FocusInfo;					  // SRanipal_Eyes_Enums.h

	int64_t TimestampRef = 0; // reference timestamp (ms) since the hmd started ticking
	struct CustomEyeData
	{
		int64_t TimestampSR = 0;   // timestamp of when the frame was captured by SRanipal. in Seconds
		int64_t TimestampUE4 = 0;  // Timestamp within UE4 of when the EyeTracker Tick() occurred
		int64_t FrameSequence = 0; // Frame seqence given by SRanipal
		// (Low level) Eye Tracker Data
		FVector GazeRay{FVector::ZeroVector}; // initialized as {1,0,0}
		FVector EyeOrigin{-5.60, 0.14, 1.12}; // initialized to {0,0,0} at the camera
		bool GazeValid = false;
		float Vergence = -1.0f; // distance to focus point
		// L/R additional eye tracker data from SRanipal
		FVector LGazeRay{FVector::ZeroVector};	  // Left eye Gaze ray only
		FVector LEyeOrigin{-6.308, 3.247, 1.264}; // initialized to {0,0,0} at the camera
		bool LGazeValid = false;
		FVector RGazeRay{FVector::ZeroVector};	   // Right eye Gaze ray only
		FVector REyeOrigin{-5.284, -3.269, 1.014}; // initialized to {0,0,0} at the camera
		bool RGazeValid = false;
		// Eye openness
		float LEyeOpenness = -1.0f; // Left eye openness (invalid at first)
		bool LEyeOpenValid = false;
		float REyeOpenness = -1.0f; // Right eye openness (invalid at first)
		bool REyeOpenValid = false;
		// Pupil positions
		// (0, 0) is center, (1, 1) is top right, (-1, -1) is bottom left
		FVector2D LPupilPos{FVector2D::ZeroVector}; // left eye pupil position
		bool LPupilPosValid = false;
		FVector2D RPupilPos{FVector2D::ZeroVector}; // right eye pupil position
		bool RPupilPosValid = false;
		// Pupil diameter
		float LPupilDiameter = 0.0; // Left pupil diameter in mm
		float RPupilDiameter = 0.0; // Right pupil diameter in mm
		// Combined data convergence mm
		float ConvergenceDist; // convergence distance (in mm)
		// FFocusInfo
		FString FocusActorName;	 // Tag of the actor being focused on
		FVector FocusActorPoint; // Hit point of the Focus Actor
		float FocusActorDist;	 // Distance to the Focus Actor
	};

	std::vector<CustomEyeData> AllData;
	// non-SRanipal data fields
};