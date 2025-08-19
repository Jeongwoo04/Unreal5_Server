// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void US1SkillSlot::SetSkill(UTexture2D* InIcon, const FText& InKey)
{
    CurrentIcon = InIcon;
    CurrentKey = InKey;

    if (Img_Icon && CurrentIcon)
    {
        Img_Icon->SetBrushFromTexture(CurrentIcon);
    }
    if (Txt_Key)
    {
        Txt_Key->SetText(CurrentKey);
    }
}

void US1SkillSlot::StartCooldown(float CooldownTime)
{
    Cooldown = CooldownTime;
    RemainCooldown = CooldownTime;
    bIsOnCooldown = true;

    if (ProgressBar)
    {
        ProgressBar->SetPercent(1.0f);
        ProgressBar->SetVisibility(ESlateVisibility::Visible);
    }
}

void US1SkillSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsOnCooldown)
    {
        RemainCooldown -= InDeltaTime;
        float Ratio = FMath::Clamp(RemainCooldown / Cooldown, 0.f, 1.f);

        if (ProgressBar)
        {
            ProgressBar->SetPercent(Ratio);
        }

        if (RemainCooldown <= 0.f)
        {
            bIsOnCooldown = false;
            if (ProgressBar)
            {
                ProgressBar->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }
}

void US1SkillSlot::NativePreConstruct()
{
    Super::NativePreConstruct();

    // ÇÁ¸®ºä¿ë
    if (PreviewIcon)
    {
        Img_Icon->SetBrushFromTexture(PreviewIcon);
    }
    if (!PreviewKey.IsEmpty())
    {
        Txt_Key->SetText(PreviewKey);
    }
}