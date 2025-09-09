// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/S1Player.h"
#include "InputActionValue.h"
#include "S1MyPlayer.generated.h"

/**
 * 
 */
class US1LoadoutComponent;
class US1SkillComponent;
class US1SkillBar;
class AS1MarkerActor;
class AS1PlayerController;
class UInputAction;

UCLASS()
class S1_API AS1MyPlayer : public AS1Player
{
	GENERATED_BODY()

public:
	AS1MyPlayer();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Tick(float DeltaTime) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	void InputRightClickMove(const FInputActionValue& value);
	void SpawnClickFX(const FVector& Location);

	void OnSkillSlotPressed(int32 SlotIndex);
	void OnSkillSlotReleased(int32 SlotIndex);

	void OnSkillSlot1Pressed();
	void OnSkillSlot2Pressed();
	void OnSkillSlot3Pressed();
	void OnSkillSlot4Pressed();

	void OnSkillSlot1Released();
	void OnSkillSlot2Released();
	void OnSkillSlot3Released();
	void OnSkillSlot4Released();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent* CameraRoot;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RightClickMoveAction;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	US1LoadoutComponent* LoadoutComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	US1SkillComponent* SkillComponent;

	UPROPERTY()
	US1SkillBar* SkillBar;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Skill1Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Skill2Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Skill3Action;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Skill4Action;

protected:
	// 클릭 마커
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker")
	UMaterialInterface* ClickMarkerMaterial;

	UPROPERTY()
	AS1MarkerActor* ClickMarker;

public:
	// 클릭 마커
	void SpawnClickMarker(const FVector& Location);

public:
	void InitSkillBar();
	void BindSkillBar(US1SkillBar* InSkillBar);
	void PossessedBy(AController* NewController);
	void TrySetupInput(AS1PlayerController* PC);
	UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }
	void SetState(const Protocol::StateMachine& State) { PosInfo.set_state(State); }

protected:
	const float MOVE_PACKET_SEND_DELAY = 0.2f;
	float MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

	FVector ClickTargetLocation;

	bool DirtyFlag = false;
	float MoveSendInterval = 0.1f;
	float TimeSinceLastSend = 0.f;

	float SkillCooldown = 0.f; // TODO : skill data 처리
	float TimeSinceLastSkill = 0.f;

	void SendMovePacket();
};
