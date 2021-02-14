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
	SelfCamera = pc->PlayerCameraManager;
	// Now we'll begin with setting up the VR Origin logic
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye); // Also have Floor & Stage Level
	
	check(SRanipal != nullptr);
	SRanipalFramework->StartFramework(SupportedEyeVersion::version1); // same result with version 2

	SRanipal->GetEyeData_(&EyeData);
    TimestampRef = EyeData.timestamp;
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
	// Getting real eye tracker data
	SRanipal->GetEyeData_(&EyeData);
    TimestampSR = EyeData.timestamp - TimestampRef;
	// ftime_s is used to get the UE4 (carla) timestamp of the world at this tick
    double ftime_s = UGameplayStatics::GetRealTimeSeconds(World);
    // Assigns EyeOrigin and Gaze direction (normalized) of combined gaze
    GazeValid = SRanipal->GetGazeRay(GazeIndex::COMBINE, EyeOrigin, GazeRay);
    // Assign Left/Right Gaze direction
    /// NOTE: the eye gazes are reversed at the lowest level bc SRanipal has a bug in their
    // libraries that flips these when collected from the sensor. We can verify this by
    // plotting debug lines for the left and right eye gazes and notice that if we close
    // one eye, the OTHER eye's gaze is null, when it should be the closed eye instead
    LGazeValid = SRanipal->GetGazeRay(GazeIndex::LEFT, LEyeOrigin, RGazeRay);
    RGazeValid = SRanipal->GetGazeRay(GazeIndex::RIGHT, REyeOrigin, LGazeRay);
    // Assign Eye openness
    LEyeOpenValid = SRanipal->GetEyeOpenness(EyeIndex::LEFT, LEyeOpenness);
    REyeOpenValid = SRanipal->GetEyeOpenness(EyeIndex::RIGHT, REyeOpenness);
    // Assign Pupil positions
    LPupilPosValid = SRanipal->GetPupilPosition(EyeIndex::LEFT, LPupilPos);
    RPupilPosValid = SRanipal->GetPupilPosition(EyeIndex::RIGHT, RPupilPos);
    // Assign Pupil Diameters
    LPupilDiameter = EyeData.verbose_data.left.pupil_diameter_mm;
    RPupilDiameter = EyeData.verbose_data.right.pupil_diameter_mm;
    // Assign data convergence
    ConvergenceDist = EyeData.verbose_data.combined.convergence_distance_mm;
    // Assign FFocus information
    /// NOTE: requires the player's camera manager
    /// NOTE: the ECollisionChannel::ECC_Pawn is set to ignore the pawn/vehicle for the line-trace
    FVector FocusOrigin, FocusDirection; // These are ignored since we compute them in EgoVehicle
    SRanipal->Focus(GazeIndex::COMBINE, SelfCamera,
                    ECC_GameTraceChannel4, FocusInfo,
                    FocusOrigin, FocusDirection);
    if(FocusInfo.actor != nullptr){
        FocusInfo.actor->GetName(FocusActorName);
    }
    else{
        FocusActorName = FString(""); // empty string, not looking at any actor
    }
    UE_LOG(LogTemp, Log, TEXT("Focus Actor: %s"), *FocusActorName);
    FocusActorPoint = FocusInfo.point;
    FocusActorDist = FocusInfo.distance;
	// Update the UE4 tick timestamp
    TimestampUE4 = int64_t(ftime_s * 1000);
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

