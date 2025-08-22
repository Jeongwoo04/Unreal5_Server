// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/S1Player.h"
#include "InputActionValue.h"
#include "S1PlayerController.h"
#include "S1LoadoutComponent.h"
#include "S1MyPlayer.generated.h"

/**
 * 
 */
struct Skill;
class US1SkillBar;

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
	void TickServerMove(float DeltaTime) override { }

	void InputMove(const FInputActionValue& Value);
	void InputLook(const FInputActionValue& Value);

	void UseSkillSlot(int32 SlotIndex);

	void UseSkillSlot1();
	void UseSkillSlot2();
	void UseSkillSlot3();
	void UseSkillSlot4();

protected:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	US1LoadoutComponent* LoadoutComponent;

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

	// Cache
	FVector2D CacheVector;
	FVector2D InputVector;

	bool DirtyFlag = false;
	float MoveSendInterval = 0.1f;
	float TimeSinceLastSend = 0.f;

	float SkillCooldown = 0.f; // TODO : skill data Ã³¸®
	float TimeSinceLastSkill = 0.f;

	void SendMovePacket();
};
