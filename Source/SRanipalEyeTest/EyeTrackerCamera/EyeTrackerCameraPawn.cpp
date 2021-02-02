// Fill out your copyright notice in the Description page of Project Settings.


#include "EyeTrackerCameraPawn.h"
#include "Kismet/KismetSystemLibrary.h"		   // PrintString, QuitGame
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"			   // GetPlayerController
#include "DrawDebugHelpers.h"				   // Debug Line/Sphere


// Sets default values
AEyeTrackerCameraPawn::AEyeTrackerCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set this pawn to be controlled by first (only) player
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Spawn the RootComponent and Camera for the VR camera
	VRCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraRoot"));
	VRCameraRoot->SetupAttachment(GetRootComponent());		// The vehicle blueprint itself

	// Create a camera and attach to root component
	FirstPersonCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCam"));
	FirstPersonCam->SetupAttachment(VRCameraRoot);
	FirstPersonCam->bUsePawnControlRotation = false; // free for VR movement
	FirstPersonCam->FieldOfView = 90.0f;			 // editable

    // Initialize the SRanipal eye tracker (WINDOWS ONLY)
	SRanipalFramework = SRanipalEye_Framework::Instance();
    SRanipal = SRanipalEye_Core::Instance();
}
void AEyeTrackerCameraPawn::ErrMsg(const FString &message, const bool isFatal = false)
{
	/// NOTE: solely for debugging
	UKismetSystemLibrary::PrintString(World, message, true, true, FLinearColor(1, 0, 0, 1), 20.0f);
	if (isFatal)
		UKismetSystemLibrary::QuitGame(World, pc, EQuitPreference::Quit, false);
	return;
}

// Called when the game starts or when spawned
void AEyeTrackerCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// get the world
    World = GetWorld();

	pc = UGameplayStatics::GetPlayerController(World, 0); // main player (0) controller

	// Now we'll begin with setting up the VR Origin logic
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye); // Also have Floor & Stage Level
	
	check(SRanipal != nullptr);
	SRanipalFramework->StartFramework(SupportedEyeVersion::version1); // same result with version 2
}

void AEyeTrackerCameraPawn::BeginDestroy()
{
    if (SRanipal)
        SRanipalEye_Core::DestroyEyeModule();
	if (SRanipalFramework){
		SRanipalFramework->StopFramework();
		SRanipalEye_Framework::DestroyEyeFramework();
	}
    Super::BeginDestroy();
}

// Called every frame
void AEyeTrackerCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    // Assign Left/Right Gaze direction
	check(SRanipal != nullptr);
    bool validL = SRanipal->GetGazeRay(GazeIndex::LEFT, LEyeOrigin, LGazeRay);
    bool validR = SRanipal->GetGazeRay(GazeIndex::RIGHT, REyeOrigin, RGazeRay);
	// get information about the VR world
	const float ScaleToUE4Meters = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(World);
	FRotator WorldRot = FirstPersonCam->GetComponentRotation(); // based on the hmd rotation
	// Now finally for the interesting part:
	// Draw individual rays for left (green) and right (yellow) eye
	FVector LeftEyeGaze = WorldRot.RotateVector(ScaleToUE4Meters * LGazeRay);
	FVector LeftEyeOrigin = WorldRot.RotateVector(LEyeOrigin) + FirstPersonCam->GetComponentLocation();
	DrawDebugLine(World, LeftEyeOrigin, LeftEyeOrigin + LeftEyeGaze, FColor::Green, false, -1, 0, 1);

	FVector RightEyeGaze = WorldRot.RotateVector(ScaleToUE4Meters * RGazeRay);
	FVector RightEyeOrigin = WorldRot.RotateVector(REyeOrigin) + FirstPersonCam->GetComponentLocation();
	DrawDebugLine(World, RightEyeOrigin, RightEyeOrigin + RightEyeGaze, FColor::Yellow, false, -1, 0, 1);

}

// Called to bind functionality to input
void AEyeTrackerCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

