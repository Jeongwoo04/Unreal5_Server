// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillBar.h"
#include "Game/S1SkillSlot.h"
#include "Data/S1DataManager.h"
#include <Kismet/KismetRenderingLibrary.h>


void US1SkillBar::NativeConstruct()
{
    Super::NativeConstruct();

    SkillSlots.Empty();
    
	// 슬롯 배열 초기화
	SkillSlots = { WBP_SkillSlot_0, WBP_SkillSlot_1, WBP_SkillSlot_2, WBP_SkillSlot_3 };

	// 단축키 배열 Q W E R
	TArray<FText> Keys = { FText::FromString("Q"), FText::FromString("W"), FText::FromString("E"), FText::FromString("R") };

    // SkillDict에서 id 1~4 가져오기
    for (int32 i = 0; i < SkillSlots.Num(); i++)
    {
        if (!SkillSlots[i]) continue;

        // DataManager에서 SkillData 가져오기
        auto SkillIt = S1DataManager::Instance().SkillDict.find(i + 1); // id 1~4
        if (SkillIt == S1DataManager::Instance().SkillDict.end())
            continue;

        auto& Skill = SkillIt->second;

        // iconPath -> UTexture2D 변환
        UTexture2D* IconTexture = nullptr;
        if (!Skill.iconPath.empty())
        {
            // std::string -> FString 변환
            FString IconPathFStr(Skill.iconPath.c_str());

            // 프로젝트 Content 디렉토리와 결합
            FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), IconPathFStr);

            // 텍스처 가져오기
            IconTexture = UKismetRenderingLibrary::ImportFileAsTexture2D(GetWorld(), FullPath);
        }

        // 슬롯 세팅
        SkillSlots[i]->SetSkill(IconTexture, Keys[i]);
    }
}

void US1SkillBar::SetSkillSlot(int32 SlotIndex, UTexture2D* InIcon, const FText& InKey)
{
    if (SkillSlots.IsValidIndex(SlotIndex))
    {
        SkillSlots[SlotIndex]->SetSkill(InIcon, InKey);
    }
}

void US1SkillBar::StartSlotCooldown(int32 SlotIndex, float CooldownTime)
{
    if (SkillSlots.IsValidIndex(SlotIndex))
    {
        SkillSlots[SlotIndex]->StartCooldown(CooldownTime);
    }
}