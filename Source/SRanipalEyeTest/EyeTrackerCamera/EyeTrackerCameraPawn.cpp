// Fill out your copyright notice in the Description page of Project Settings.

#include "EyeTrackerCameraPawn.h"
#include "Kismet/KismetSystemLibrary.h" // PrintString, QuitGame
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h" // GetPlayerController
#include "DrawDebugHelpers.h"		// Debug Line/Sphere

#include <iostream> // IO
#include <fstream>	// File IO

// Sets default values
AEyeTrackerCameraPawn::AEyeTrackerCameraPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set this pawn to be controlled by first (only) player
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Spawn the RootComponent and Camera for the VR camera
	VRCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraRoot"));
	VRCameraRoot->SetupAttachment(GetRootComponent()); // The vehicle blueprint itself

	// Create a camera and attach to root component
	FirstPersonCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCam"));
	FirstPersonCam->SetupAttachment(VRCameraRoot);
	FirstPersonCam->bUsePawnControlRotation = false; // free for VR movement
	FirstPersonCam->FieldOfView = 90.0f;			 // editable
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

	// Initialize the SRanipal eye tracker (WINDOWS ONLY)
	SRanipalFramework = SRanipalEye_Framework::Instance();
	SRanipal = SRanipalEye_Core::Instance();
	check(SRanipal != nullptr);
	SRanipalFramework->StartFramework(SupportedEyeVersion::version1); // same result with version 2

	SRanipal->GetEyeData_(&EyeData);
	TimestampRef = EyeData.timestamp;
}

void AEyeTrackerCameraPawn::BeginDestroy()
{
	WriteDataToFile();
	if (SRanipal)
		SRanipalEye_Core::DestroyEyeModule();
	if (SRanipalFramework)
	{
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
	// all the SRanipal data will be stored in this structure
	CustomEyeData ced;
	// Getting real eye tracker data
	SRanipal->GetEyeData_(&EyeData);
	ced.TimestampSR = EyeData.timestamp - TimestampRef;
	ced.FrameSequence = EyeData.frame_sequence; // add raw "frame sequence" from SRanipal Eyes Enums
												// ftime_s is used to get the UE4 (carla) timestamp of the world at this tick
	double ftime_s = UGameplayStatics::GetRealTimeSeconds(World);
	// Assigns EyeOrigin and Gaze direction (normalized) of combined gaze
	ced.GazeValid = SRanipal->GetGazeRay(GazeIndex::COMBINE, ced.EyeOrigin, ced.GazeRay);
	// Assign Left/Right Gaze direction
	/// NOTE: the eye gazes are reversed at the lowest level bc SRanipal has a bug in their
	// libraries that flips these when collected from the sensor. We can verify this by
	// plotting debug lines for the left and right eye gazes and notice that if we close
	// one eye, the OTHER eye's gaze is null, when it should be the closed eye instead
	ced.LGazeValid = SRanipal->GetGazeRay(GazeIndex::LEFT, ced.LEyeOrigin, ced.LGazeRay);
	ced.RGazeValid = SRanipal->GetGazeRay(GazeIndex::RIGHT, ced.REyeOrigin, ced.RGazeRay);
	// Assign Eye openness
	ced.LEyeOpenValid = SRanipal->GetEyeOpenness(EyeIndex::LEFT, ced.LEyeOpenness);
	ced.REyeOpenValid = SRanipal->GetEyeOpenness(EyeIndex::RIGHT, ced.REyeOpenness);
	// Assign Pupil positions
	ced.LPupilPosValid = SRanipal->GetPupilPosition(EyeIndex::LEFT, ced.LPupilPos);
	ced.RPupilPosValid = SRanipal->GetPupilPosition(EyeIndex::RIGHT, ced.RPupilPos);
	// Assign Pupil Diameters
	ced.LPupilDiameter = EyeData.verbose_data.left.pupil_diameter_mm;
	ced.RPupilDiameter = EyeData.verbose_data.right.pupil_diameter_mm;
	// Assign data convergence
	ced.ConvergenceDist = EyeData.verbose_data.combined.convergence_distance_mm;
	// Assign FFocus information
	/// NOTE: requires the player's camera manager
	/// NOTE: the ECollisionChannel::ECC_Pawn is set to ignore the pawn/vehicle for the line-trace
	FVector FocusOrigin, FocusDirection; // These are ignored since we compute them in EgoVehicle
	SRanipal->Focus(GazeIndex::COMBINE, pc->PlayerCameraManager,
					ECC_Pawn, FocusInfo,
					FocusOrigin, FocusDirection);
	if (FocusInfo.actor != nullptr)
		FocusInfo.actor->GetName(ced.FocusActorName);
	else
		ced.FocusActorName = FString(""); // empty string, not looking at any actor
	ced.FocusActorPoint = FocusInfo.point;
	ced.FocusActorDist = FocusInfo.distance;
	// Update the UE4 tick timestamp
	ced.TimestampUE4 = int64_t(ftime_s * 1000);
	// get information about the VR world
	const float ScaleToUE4Meters = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(World);
	FRotator WorldRot = FirstPersonCam->GetComponentRotation(); // based on the hmd rotation
	// Now finally for the interesting part:
	// Draw individual rays for left (green) and right (yellow) eye
	FVector LeftEyeGaze = WorldRot.RotateVector(ScaleToUE4Meters * ced.LGazeRay);
	FVector LeftEyeOrigin = WorldRot.RotateVector(ced.LEyeOrigin) + FirstPersonCam->GetComponentLocation();
	DrawDebugLine(World, LeftEyeOrigin, LeftEyeOrigin + LeftEyeGaze, FColor::Green, false, -1, 0, 1);

	FVector RightEyeGaze = WorldRot.RotateVector(ScaleToUE4Meters * ced.RGazeRay);
	FVector RightEyeOrigin = WorldRot.RotateVector(ced.REyeOrigin) + FirstPersonCam->GetComponentLocation();
	DrawDebugLine(World, RightEyeOrigin, RightEyeOrigin + RightEyeGaze, FColor::Yellow, false, -1, 0, 1);

	// append the data to our cumulative bookkeeping
	AllData.push_back(ced);
}

// Called to bind functionality to input
void AEyeTrackerCameraPawn::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

template <typename T>
void WriteToFile(const std::vector<T> &data, const std::string &name)
{
	std::ofstream FILE(name + ".txt");
	for (auto x : data)
	{
		FILE << x << " ";
	}
	FILE.close();
}

void AEyeTrackerCameraPawn::WriteDataToFile() const
{
	std::vector<float> TimesSR, TimesUE4;
	std::vector<int64_t> FrameSeq;
	// preprocess all data into vector of floats for easy printings
	for (auto ced : AllData)
	{
		TimesSR.push_back(ced.TimestampSR / 1000.0f);
		TimesUE4.push_back(ced.TimestampUE4 / 1000.0f);
		FrameSeq.push_back(ced.FrameSequence);
	}
	// write to file
	WriteToFile(TimesSR, "test_sranipal");
	WriteToFile(TimesUE4, "test_ue4");
	WriteToFile(FrameSeq, "test_frameseq");
}