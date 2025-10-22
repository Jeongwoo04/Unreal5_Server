// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void US1SkillSlot::SetSkill(UTexture2D* InIcon, const FText& InKey)
{
    if (Img_Icon && InIcon)
    {
        Img_Icon->SetBrushFromTexture(InIcon);
    }
    if (Txt_Key)
    {
        Txt_Key->SetText(InKey);
    }
}

void US1SkillSlot::StartCooldown(uint64 CooldownTick)
{
    CooldownDurationTick = CooldownTick;
    bIsOnCooldown = true;
    CooldownStartTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);

    if (ProgressBar)
    {
        ProgressBar->SetPercent(1.0f);
        ProgressBar->SetVisibility(ESlateVisibility::Visible);
    }
}

bool US1SkillSlot::CanUseSkill()
{
    return !bIsOnCooldown;
}

void US1SkillSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsOnCooldown)
    {
        uint64 CurrentTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);
        uint64 Elapsed = CurrentTick - CooldownStartTick;

        float Ratio = FMath::Clamp(1.f - float(Elapsed) / float(CooldownDurationTick), 0.f, 1.f);

        if (ProgressBar)
            ProgressBar->SetPercent(Ratio);

        if (Elapsed >= CooldownDurationTick)
        {
            bIsOnCooldown = false;
            if (ProgressBar)
                ProgressBar->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    //if (bIsOnCooldown)
    //{
    //    RemainCooldown -= InDeltaTime;
    //    float Ratio = FMath::Clamp(RemainCooldown / Cooldown, 0.f, 1.f);

    //    if (ProgressBar)
    //    {
    //        ProgressBar->SetPercent(Ratio);
    //    }

    //    if (RemainCooldown <= 0.f)
    //    {
    //        bIsOnCooldown = false;
    //        if (ProgressBar)
    //        {
    //            ProgressBar->SetVisibility(ESlateVisibility::Hidden);
    //        }
    //    }
    //}
}