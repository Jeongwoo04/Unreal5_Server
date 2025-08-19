// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S1SkillSlot.generated.h"

/**
 * 
 */
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;

UCLASS()
class S1_API US1SkillSlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Skill")
	void SetSkill(UTexture2D* InIcon, const FText& InKey);

	UFUNCTION(BlueprintCallable, Category="Skill")
	void StartCooldown(float CooldownTime);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void NativePreConstruct() override;

protected:
	UPROPERTY(meta=(BindWidget))
	UImage* Img_Icon;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* Txt_Key;

	UPROPERTY(meta=(BindWidget))
	UProgressBar* ProgressBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Skill")
	TObjectPtr<UTexture2D> CurrentIcon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Skill")
	FText CurrentKey;

	bool bIsOnCooldown = false;
	float Cooldown = 0.f;
	float RemainCooldown = 0.f;

	UPROPERTY(EditAnywhere, Category="Preview")
	TObjectPtr<UTexture2D> PreviewIcon;

	UPROPERTY(EditAnywhere, Category="Preview")
	FText PreviewKey;
};
