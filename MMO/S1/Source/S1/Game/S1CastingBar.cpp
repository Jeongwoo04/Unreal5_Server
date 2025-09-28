// Fill out your copyright notice in the Description page of Project Settings.


#include "S1CastingBar.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "S1SkillComponent.h"

void US1CastingBar::ShowCastingBar(const FSkillState& CurrentState, uint64 ServerCastEndTick)
{
    SetVisibility(ESlateVisibility::Visible);
    CastingBar_Fill->SetPercent(0.f);
    CastingBar_Text->SetText(FText::FromString(CurrentState.name));

    TotalCastTime = CurrentState.CastTime;

    if (ServerCastEndTick > 0)
    {
        float ServerEndTime = static_cast<float>(ServerCastEndTick) / 1000.f;
        float ClientTime = GetWorld()->GetTimeSeconds();
        CurrentCastTime = FMath::Clamp(TotalCastTime - (ServerEndTime - ClientTime), 0.f, TotalCastTime);
    }
    else
    {
        CurrentCastTime = 0.f;
    }
}

void US1CastingBar::CancelCasting()
{
    SetVisibility(ESlateVisibility::Hidden);
    CurrentCastTime = 0.f;
    TotalCastTime = 0.f;
}

void US1CastingBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (TotalCastTime > 0.f)
    {
        CurrentCastTime += InDeltaTime;
        float Percent = FMath::Clamp(CurrentCastTime / TotalCastTime, 0.f, 1.f);
        CastingBar_Fill->SetPercent(Percent);

        if (Percent >= 1.f)
        {
            SetVisibility(ESlateVisibility::Hidden);
            TotalCastTime = 0.f;
        }
    }
}
