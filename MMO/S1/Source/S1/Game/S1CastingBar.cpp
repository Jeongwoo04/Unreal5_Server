// Fill out your copyright notice in the Description page of Project Settings.


#include "S1CastingBar.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "S1SkillComponent.h"

void US1CastingBar::StartCasting(const FSkillState& SkillState, uint64 ClientCastEndTick)
{
    SetVisibility(ESlateVisibility::Visible);
    CastingBar_Fill->SetPercent(0.f);
    CastingBar_Text->SetText(FText::FromString(SkillState.name));

    InitializeCast(SkillState, ClientCastEndTick);
}

void US1CastingBar::InitializeCast(const FSkillState& SkillState, uint64 ClientCastEndTick)
{
    CastDuration = SkillState.CastTime;

    // 서버 보정이 있는 경우
    if (ClientCastEndTick > 0)
    {
        float ClientTimeSec = FPlatformTime::Seconds(); // 플랫폼 타임 기준
        float ServerEndTimeSec = (float)ClientCastEndTick / 1000.f;

        // 경과시간 = 총시간 - 남은시간
        ElapsedCastTime = FMath::Clamp(CastDuration - (ServerEndTimeSec - ClientTimeSec), 0.f, CastDuration);
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
    ElapsedCastTime += DeltaTime;
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