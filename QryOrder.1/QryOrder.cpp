// 标准C库文件
#include <stdio.h>
#include <string.h>
#include <cstdlib>

// CTP头文件
#include <ThostFtdcTraderApi.h>
#include <ThostFtdcMdApi.h>
#include <ThostFtdcUserApiDataType.h>
#include <ThostFtdcUserApiStruct.h>

// 线程控制相关
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// 字符串编码转化
#include <code_convert.h>

// 登录请求结构体
CThostFtdcReqUserLoginField userLoginField;
// 用户请求结构体
CThostFtdcUserLogoutField userLogoutField;
// 线程同步标志
sem_t sem;
// requestID
int requestID = 0;


class CTraderHandler : public CThostFtdcTraderSpi {

public:

    CTraderHandler() {
        printf("CTraderHandler():被执行...\n");
    }

    // 允许登录事件
    virtual void OnFrontConnected() {
        static int i = 0;
        printf("OnFrontConnected():被执行...\n");
        // 在登出后系统会重新调用OnFrontConnected，这里简单判断并忽略第1次之后的所有调用。
        if (i++==0) {
            sem_post(&sem);
        }
    }

    // 登录结果响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        printf("OnRspUserLogin():被执行...\n");
        if (pRspInfo->ErrorID == 0) {
            printf("登录成功!\n");
            sem_post(&sem);
        } else {
            printf("登录失败!\n");
        }
    }

    // 登出结果响应
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        printf("OnReqUserLogout():被执行...\n");
        if (pRspInfo->ErrorID == 0) {
            printf("登出成功!\n");
            sem_post(&sem);
        } else {
            printf("登出失败!\n");
        }
    }

    // 错误信息响应方法
    virtual void OnRspError
    (CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        printf("OnRspError():被执行...\n");
    }

    ///请求查询报单响应
    virtual void OnRspQryOrder(
        CThostFtdcOrderField * pOrder,
        CThostFtdcRspInfoField * pRspInfo,
        int nRequestID,
        bool bIsLast
    ) {
        printf("OnRspQryOrder():被执行...\n");

        // 进程返回结果检查
        if ( (pRspInfo) && (pRspInfo->ErrorID != 0) )  {
            // typedef int TThostFtdcErrorIDType;
            // typedef char TThostFtdcErrorMsgType[81];
            char ErrorMsg[243];
            gbk2utf8(pRspInfo->ErrorMsg,ErrorMsg,sizeof(ErrorMsg));
            printf("OnRspQryOrder():出错:ErrorId=%d,ErrorMsg=%s\n",pRspInfo->ErrorID,ErrorMsg);
        }

        // 如果有返回结果读取返回信息
        if ( pOrder != NULL ) {
            // 读取返回信息,并做编码转化
            ///经纪公司代码 TThostFtdcBrokerIDType char[11]
            char BrokerID[33];
            gbk2utf8(pOrder->BrokerID,BrokerID,sizeof(BrokerID));
            ///投资者代码 TThostFtdcInvestorIDType char[13]
            char InvestorID[39];
            gbk2utf8(pOrder->InvestorID,InvestorID,sizeof(InvestorID));
            ///合约代码 TThostFtdcInstrumentIDType char[31]
            char InstrumentID[93];
            gbk2utf8(pOrder->InstrumentID,InstrumentID,sizeof(InstrumentID));
            ///报单引用 TThostFtdcOrderRefType char[13]
            char OrderRef[39];
            gbk2utf8(pOrder->OrderRef,OrderRef,sizeof(OrderRef));
            ///用户代码 TThostFtdcUserIDType char[16]
            char UserID[48];
            gbk2utf8(pOrder->UserID,UserID,sizeof(UserID));
            ///报单价格条件 TThostFtdcOrderPriceTypeType char
            //// THOST_FTDC_OPT_AnyPrice '1' 任意价
            //// THOST_FTDC_OPT_LimitPrice '2' 限价
            //// THOST_FTDC_OPT_BestPrice '3' 最优价
            //// THOST_FTDC_OPT_LastPrice '4' 最新价
            //// THOST_FTDC_OPT_LastPricePlusOneTicks '5' 最新价浮动上浮1个ticks
            //// THOST_FTDC_OPT_LastPricePlusTwoTicks '6' 最新价浮动上浮2个ticks
            //// THOST_FTDC_OPT_LastPricePlusThreeTicks '7' 最新价浮动上浮3个ticks
            //// THOST_FTDC_OPT_AskPrice1 '8' 卖一价
            //// THOST_FTDC_OPT_AskPrice1PlusOneTicks '9' 卖一价浮动上浮1个ticks
            //// THOST_FTDC_OPT_AskPrice1PlusTwoTicks 'A' 卖一价浮动上浮2个ticks
            //// THOST_FTDC_OPT_AskPrice1PlusThreeTicks 'B' 卖一价浮动上浮3个ticks
            //// THOST_FTDC_OPT_BidPrice1 'C' 买一价
            //// THOST_FTDC_OPT_BidPrice1PlusOneTicks 'D' 买一价浮动上浮1个ticks
            //// THOST_FTDC_OPT_BidPrice1PlusTwoTicks 'E' 买一价浮动上浮2个ticks
            //// THOST_FTDC_OPT_BidPrice1PlusThreeTicks 'F' 买一价浮动上浮3个ticks
            char OrderPriceType = pOrder->OrderPriceType;
            ///买卖方向 TThostFtdcDirectionType char
            //// THOST_FTDC_D_Buy '0' 买
            //// THOST_FTDC_D_Sell '1' 卖
            char Direction = pOrder->Direction;
            ///组合开平标志 TThostFtdcCombOffsetFlagType char[5]
            char CombOffsetFlag[15];
            gbk2utf8(pOrder->CombOffsetFlag,CombOffsetFlag,sizeof(CombOffsetFlag));
            ///组合投机套保标志 TThostFtdcCombHedgeFlagType char[5]
            char CombHedgeFlag[15];
            gbk2utf8(pOrder->CombHedgeFlag,CombHedgeFlag,sizeof(CombHedgeFlag));
            ///价格 TThostFtdcPriceType double
            double LimitPrice = pOrder->LimitPrice;
            ///数量 TThostFtdcVolumeType int
            int VolumeTotalOriginal = pOrder->VolumeTotalOriginal;
            ///有效期类型 TThostFtdcTimeConditionType char
            //// THOST_FTDC_TC_IOC '1' 立即完成，否则撤销
            //// THOST_FTDC_TC_GFS '2' 本节有效
            //// THOST_FTDC_TC_GFD '3' 当日有效
            //// THOST_FTDC_TC_GTD '4' 指定日期前有效
            //// THOST_FTDC_TC_GTC '5' 撤销前有效
            //// THOST_FTDC_TC_GFA '6' 集合竞价有效
            char TimeCondition = pOrder->TimeCondition;
            ///GTD日期 TThostFtdcDateType char[9]
            char GTDDate[27];
            gbk2utf8(pOrder->GTDDate,GTDDate,sizeof(GTDDate));
            ///成交量类型 TThostFtdcVolumeConditionType char
            //// THOST_FTDC_VC_AV '1' 任何数量
            //// THOST_FTDC_VC_MV '2' 最小数量
            //// THOST_FTDC_VC_CV '3' 全部数量
            char VolumeCondition = pOrder->VolumeCondition;
            ///最小成交量 TThostFtdcVolumeType int
            int MinVolume = pOrder->MinVolume;
            ///触发条件 TThostFtdcContingentConditionType char
            //// THOST_FTDC_CC_Immediately '1' 立即
            //// THOST_FTDC_CC_Touch '2' 止损
            //// THOST_FTDC_CC_TouchProfit '3' 止赢
            //// THOST_FTDC_CC_ParkedOrder '4' 预埋单
            //// THOST_FTDC_CC_LastPriceGreaterThanStopPrice '5' 最新价大于条件价
            //// THOST_FTDC_CC_LastPriceGreaterEqualStopPrice '6' 最新价大于等于条件价
            //// THOST_FTDC_CC_LastPriceLesserThanStopPrice '7' 最新价小于条件价
            //// THOST_FTDC_CC_LastPriceLesserEqualStopPrice '8' 最新价小于等于条件价
            //// THOST_FTDC_CC_AskPriceGreaterThanStopPrice '9' 卖一价大于条件价
            //// THOST_FTDC_CC_AskPriceGreaterEqualStopPrice 'A' 卖一价大于等于条件价
            //// THOST_FTDC_CC_AskPriceLesserThanStopPrice 'B' 卖一价小于条件价
            //// THOST_FTDC_CC_AskPriceLesserEqualStopPrice 'C' 卖一价小于等于条件价
            //// THOST_FTDC_CC_BidPriceGreaterThanStopPrice 'D' 买一价大于条件价
            //// THOST_FTDC_CC_BidPriceGreaterEqualStopPrice 'E' 买一价大于等于条件价
            //// THOST_FTDC_CC_BidPriceLesserThanStopPrice 'F' 买一价小于条件价
            //// THOST_FTDC_CC_BidPriceLesserEqualStopPrice 'H' 买一价小于等于条件价
            char ContingentCondition = pOrder->ContingentCondition;
            ///止损价 TThostFtdcPriceType double
            double StopPrice = pOrder->StopPrice;
            ///强平原因 TThostFtdcForceCloseReasonType char
            //// THOST_FTDC_FCC_NotForceClose '0' 非强平
            //// THOST_FTDC_FCC_LackDeposit '1' 资金不足
            //// THOST_FTDC_FCC_ClientOverPositionLimit '2' 客户超仓
            //// THOST_FTDC_FCC_MemberOverPositionLimit '3' 会员超仓
            //// THOST_FTDC_FCC_NotMultiple '4' 持仓非整数倍
            //// THOST_FTDC_FCC_Violation '5' 违规
            //// THOST_FTDC_FCC_Other '6' 其它
            //// THOST_FTDC_FCC_PersonDeliv '7' 自然人临近交割
            char ForceCloseReason = pOrder->ForceCloseReason;
            ///自动挂起标志 TThostFtdcBoolType int
            int IsAutoSuspend = pOrder->IsAutoSuspend;
            ///业务单元 TThostFtdcBusinessUnitType char[21]
            char BusinessUnit[63];
            gbk2utf8(pOrder->BusinessUnit,BusinessUnit,sizeof(BusinessUnit));
            ///请求编号 TThostFtdcRequestIDType int
            int RequestID = pOrder->RequestID;
            ///本地报单编号 TThostFtdcOrderLocalIDType char[13]
            char OrderLocalID[39];
            gbk2utf8(pOrder->OrderLocalID,OrderLocalID,sizeof(OrderLocalID));
            ///交易所代码 TThostFtdcExchangeIDType char[9]
            char ExchangeID[27];
            gbk2utf8(pOrder->ExchangeID,ExchangeID,sizeof(ExchangeID));
            ///会员代码 TThostFtdcParticipantIDType char[11]
            char ParticipantID[33];
            gbk2utf8(pOrder->ParticipantID,ParticipantID,sizeof(ParticipantID));
            ///客户代码 TThostFtdcClientIDType char[11]
            char ClientID[33];
            gbk2utf8(pOrder->ClientID,ClientID,sizeof(ClientID));
            ///合约在交易所的代码 TThostFtdcExchangeInstIDType char[31]
            char ExchangeInstID[93];
            gbk2utf8(pOrder->ExchangeInstID,ExchangeInstID,sizeof(ExchangeInstID));
            ///交易所交易员代码 TThostFtdcTraderIDType char[21]
            char TraderID[63];
            gbk2utf8(pOrder->TraderID,TraderID,sizeof(TraderID));
            ///安装编号 TThostFtdcInstallIDType int
            int InstallID = pOrder->InstallID;
            ///报单提交状态 TThostFtdcOrderSubmitStatusType char
            //// THOST_FTDC_OSS_InsertSubmitted '0' 已经提交
            //// THOST_FTDC_OSS_CancelSubmitted '1' 撤单已经提交
            //// THOST_FTDC_OSS_ModifySubmitted '2' 修改已经提交
            //// THOST_FTDC_OSS_Accepted '3' 已经接受
            //// THOST_FTDC_OSS_InsertRejected '4' 报单已经被拒绝
            //// THOST_FTDC_OSS_CancelRejected '5' 撤单已经被拒绝
            //// THOST_FTDC_OSS_ModifyRejected '6' 改单已经被拒绝
            char OrderSubmitStatus = pOrder->OrderSubmitStatus;
            ///报单提示序号 TThostFtdcSequenceNoType int
            int NotifySequence = pOrder->NotifySequence;
            ///交易日 TThostFtdcDateType char[9]
            char TradingDay[27];
            gbk2utf8(pOrder->TradingDay,TradingDay,sizeof(TradingDay));
            ///结算编号 TThostFtdcSettlementIDType int
            int SettlementID = pOrder->SettlementID;
            ///报单编号 TThostFtdcOrderSysIDType char[21]
            char OrderSysID[63];
            gbk2utf8(pOrder->OrderSysID,OrderSysID,sizeof(OrderSysID));
            ///报单来源 TThostFtdcOrderSourceType char
            //// THOST_FTDC_OSRC_Participant '0' 来自参与者
            //// THOST_FTDC_OSRC_Administrator '1' 来自管理员
            char OrderSource = pOrder->OrderSource;
            ///报单状态 TThostFtdcOrderStatusType char
            //// THOST_FTDC_OST_AllTraded '0' 全部成交
            //// THOST_FTDC_OST_PartTradedQueueing '1' 部分成交还在队列中
            //// THOST_FTDC_OST_PartTradedNotQueueing '2' 部分成交不在队列中
            //// THOST_FTDC_OST_NoTradeQueueing '3' 未成交还在队列中
            //// THOST_FTDC_OST_NoTradeNotQueueing '4' 未成交不在队列中
            //// THOST_FTDC_OST_Canceled '5' 撤单
            //// THOST_FTDC_OST_Unknown 'a' 未知
            //// THOST_FTDC_OST_NotTouched 'b' 尚未触发
            //// THOST_FTDC_OST_Touched 'c' 已触发
            char OrderStatus = pOrder->OrderStatus;
            ///报单类型 TThostFtdcOrderTypeType char
            //// THOST_FTDC_ORDT_Normal '0' 正常
            //// THOST_FTDC_ORDT_DeriveFromQuote '1' 报价衍生
            //// THOST_FTDC_ORDT_DeriveFromCombination '2' 组合衍生
            //// THOST_FTDC_ORDT_Combination '3' 组合报单
            //// THOST_FTDC_ORDT_ConditionalOrder '4' 条件单
            //// THOST_FTDC_ORDT_Swap '5' 互换单
            char OrderType = pOrder->OrderType;
            ///今成交数量 TThostFtdcVolumeType int
            int VolumeTraded = pOrder->VolumeTraded;
            ///剩余数量 TThostFtdcVolumeType int
            int VolumeTotal = pOrder->VolumeTotal;
            ///报单日期 TThostFtdcDateType char[9]
            char InsertDate[27];
            gbk2utf8(pOrder->InsertDate,InsertDate,sizeof(InsertDate));
            ///委托时间 TThostFtdcTimeType char[9]
            char InsertTime[27];
            gbk2utf8(pOrder->InsertTime,InsertTime,sizeof(InsertTime));
            ///激活时间 TThostFtdcTimeType char[9]
            char ActiveTime[27];
            gbk2utf8(pOrder->ActiveTime,ActiveTime,sizeof(ActiveTime));
            ///挂起时间 TThostFtdcTimeType char[9]
            char SuspendTime[27];
            gbk2utf8(pOrder->SuspendTime,SuspendTime,sizeof(SuspendTime));
            ///最后修改时间 TThostFtdcTimeType char[9]
            char UpdateTime[27];
            gbk2utf8(pOrder->UpdateTime,UpdateTime,sizeof(UpdateTime));
            ///撤销时间 TThostFtdcTimeType char[9]
            char CancelTime[27];
            gbk2utf8(pOrder->CancelTime,CancelTime,sizeof(CancelTime));
            ///最后修改交易所交易员代码 TThostFtdcTraderIDType char[21]
            char ActiveTraderID[63];
            gbk2utf8(pOrder->ActiveTraderID,ActiveTraderID,sizeof(ActiveTraderID));
            ///结算会员编号 TThostFtdcParticipantIDType char[11]
            char ClearingPartID[33];
            gbk2utf8(pOrder->ClearingPartID,ClearingPartID,sizeof(ClearingPartID));
            ///序号 TThostFtdcSequenceNoType int
            int SequenceNo = pOrder->SequenceNo;
            ///前置编号 TThostFtdcFrontIDType int
            int FrontID = pOrder->FrontID;
            ///会话编号 TThostFtdcSessionIDType int
            int SessionID = pOrder->SessionID;
            ///用户端产品信息 TThostFtdcProductInfoType char[11]
            char UserProductInfo[33];
            gbk2utf8(pOrder->UserProductInfo,UserProductInfo,sizeof(UserProductInfo));
            ///状态信息 TThostFtdcErrorMsgType char[81]
            char StatusMsg[243];
            gbk2utf8(pOrder->StatusMsg,StatusMsg,sizeof(StatusMsg));
            ///用户强评标志 TThostFtdcBoolType int
            int UserForceClose = pOrder->UserForceClose;
            ///操作用户代码 TThostFtdcUserIDType char[16]
            char ActiveUserID[48];
            gbk2utf8(pOrder->ActiveUserID,ActiveUserID,sizeof(ActiveUserID));
            ///经纪公司报单编号 TThostFtdcSequenceNoType int
            int BrokerOrderSeq = pOrder->BrokerOrderSeq;
            ///相关报单 TThostFtdcOrderSysIDType char[21]
            char RelativeOrderSysID[63];
            gbk2utf8(pOrder->RelativeOrderSysID,RelativeOrderSysID,sizeof(RelativeOrderSysID));
            ///郑商所成交数量 TThostFtdcVolumeType int
            int ZCETotalTradedVolume = pOrder->ZCETotalTradedVolume;
            ///互换单标志 TThostFtdcBoolType int
            int IsSwapOrder = pOrder->IsSwapOrder;

            printf("OrderRef=%s,",OrderRef); // 报单引用
			printf("OrderPriceType=%c,",OrderPriceType); // 报单价格条件
			printf("Direction=%c,",Direction); // 买卖方向
			printf("LimitPrice=%f,",LimitPrice); // 价格
			printf("VolumeTotalOriginal=%d,",VolumeTotalOriginal); // 数量
            printf("TimeCondition=%c,",TimeCondition); // 有效期类型
			printf("OrderStatus=%c,",OrderStatus); // 报单状态
			printf("OrderSubmitStatus=%c,",OrderSubmitStatus); // 报单提交状态
			printf("OrderSysID=%s,",OrderSysID); // 报单提交状态
			printf("FrontID=%d,",FrontID); // 前置编号
			printf("SessionID=%d\n",SessionID); // 会话编号
        }

        // 如果响应函数已经返回最后一条信息
        if(bIsLast) {
            // 通知主过程，响应函数将结束
            sem_post(&sem);
        }
    }

};


int main() {

    // 初始化线程同步变量
    sem_init(&sem,0,0);

    // 从环境变量中读取登录信息
    char * CTP_FrontAddress = getenv("CTP_FrontAddress");
    if ( CTP_FrontAddress == NULL ) {
        printf("环境变量CTP_FrontAddress没有设置\n");
        return(0);
    }

    char * CTP_BrokerId = getenv("CTP_BrokerId");
    if ( CTP_BrokerId == NULL ) {
        printf("环境变量CTP_BrokerId没有设置\n");
        return(0);
    }
    strcpy(userLoginField.BrokerID,CTP_BrokerId);

    char * CTP_UserId = getenv("CTP_UserId");
    if ( CTP_UserId == NULL ) {
        printf("环境变量CTP_UserId没有设置\n");
        return(0);
    }
    strcpy(userLoginField.UserID,CTP_UserId);

    char * CTP_Password = getenv("CTP_Password");
    if ( CTP_Password == NULL ) {
        printf("环境变量CTP_Password没有设置\n");
        return(0);
    }
    strcpy(userLoginField.Password,CTP_Password);

    // 创建TraderAPI和回调响应控制器的实例
    CThostFtdcTraderApi *pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
    CTraderHandler traderHandler = CTraderHandler();
    CTraderHandler * pTraderHandler = &traderHandler;
    pTraderApi->RegisterSpi(pTraderHandler);

    // 设置服务器地址
    pTraderApi->RegisterFront(CTP_FrontAddress);
    // 链接交易系统
    pTraderApi->Init();
    // 等待服务器发出登录消息
    sem_wait(&sem);
    // 发出登陆请求
    pTraderApi->ReqUserLogin(&userLoginField, requestID++);
    // 等待登录成功消息
    sem_wait(&sem);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ///请求查询报单
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // 定义调用API的数据结构
    CThostFtdcQryOrderField requestData;
    // 确保没有初始化的数据不会被访问
    memset(&requestData,0,sizeof(requestData));
    // 为调用结构题设置参数信息
    ///经纪公司代码 TThostFtdcBrokerIDType char[11]
    strcpy(requestData.BrokerID,"");
    ///投资者代码 TThostFtdcInvestorIDType char[13]
    strcpy(requestData.InvestorID,"");
    ///合约代码 TThostFtdcInstrumentIDType char[31]
    strcpy(requestData.InstrumentID,"");
    ///交易所代码 TThostFtdcExchangeIDType char[9]
    strcpy(requestData.ExchangeID,"");
    ///报单编号 TThostFtdcOrderSysIDType char[21]
    strcpy(requestData.OrderSysID,"");
    ///开始时间 TThostFtdcTimeType char[9]
    strcpy(requestData.InsertTimeStart,"");
    ///结束时间 TThostFtdcTimeType char[9]
    strcpy(requestData.InsertTimeEnd,"");


    // 调用API,并等待响应函数返回
    int result = pTraderApi->ReqQryOrder(&requestData,requestID++);
    sem_wait(&sem);

    /////////////////////////////////////////////////////////////////////////////////////////////////


    // 拷贝用户登录信息到登出信息
    strcpy(userLogoutField.BrokerID,userLoginField.BrokerID);
    strcpy(userLogoutField.UserID, userLoginField.UserID);
    pTraderApi->ReqUserLogout(&userLogoutField, requestID++);

    // 等待登出成功
    sem_wait(&sem);

    printf("主线程执行完毕!\n");
    return(0);

}
