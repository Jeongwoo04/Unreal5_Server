// Fill out your copyright notice in the Description page of Project Settings.


#include "S1CastingBar.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "S1SkillComponent.h"

void US1CastingBar::StartCasting(const FSkillState& SkillState, uint64 CastEndTick)
{
    SetVisibility(ESlateVisibility::Visible);
    CastingBar_Fill->SetPercent(0.f);
    CastingBar_Text->SetText(FText::FromString(SkillState.name));

    ElapsedCastTime = 0.f;

    InitializeCast(SkillState, CastEndTick);
}

void US1CastingBar::InitializeCast(const FSkillState& SkillState, uint64 CastEndTick)
{
    CastDuration = SkillState.CastTime * 1000.f;

    // 서버 보정이 있는 경우
    if (CastEndTick > 0)
    {
        uint64 ClientTimeSec = FPlatformTime::Seconds() * 1000; // 플랫폼 타임 기준
        uint64 ServerEndTimeSec = CastEndTick;

        // 경과시간 = 총시간 - 남은시간
        ElapsedCastTime = FMath::Clamp(CastDuration - static_cast<float>(ServerEndTimeSec - ClientTimeSec), 0.f, CastDuration);
    }
    else
    {
        ElapsedCastTime = 0.f;
    }
    bIsCasting = true;
}

void US1CastingBar::CancelCasting()
{
    SetVisibility(ESlateVisibility::Hidden);
    ElapsedCastTime = 0.f;
    CastDuration = 0.f;
    bIsCasting = false;
}

void US1CastingBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bIsCasting || CastDuration <= 0.f)
        return;

    UpdateCastBar(InDeltaTime);
}

void US1CastingBar::UpdateCastBar(float DeltaTime)
{
    ElapsedCastTime += DeltaTime * 1000.f;
    float Percent = FMath::Clamp(ElapsedCastTime / CastDuration, 0.f, 1.f);
    CastingBar_Fill->SetPercent(Percent);

    if (Percent >= 1.f)
    {
        SetVisibility(ESlateVisibility::Hidden);
        bIsCasting = false;
        CastDuration = 0.f;
        ElapsedCastTime = 0.f;
    }
}