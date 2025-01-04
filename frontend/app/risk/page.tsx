'use client'

import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Progress } from "@/components/ui/progress"
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts'

const riskMetricsData = [
  { name: 'VaR', value: 2450 },
  { name: 'Sharpe Ratio', value: 1.8 },
  { name: 'Max Drawdown', value: 12.5 },
  { name: 'Beta', value: 0.85 },
]

const positionLimitsData = [
  { asset: 'BTC', current: 40000, limit: 50000 },
  { asset: 'ETH', current: 30000, limit: 40000 },
  { asset: 'SOL', current: 15000, limit: 25000 },
]

export default function RiskPage() {
  return (
    <div className="grid gap-6">
      <div className="flex items-center justify-between">
        <h1 className="text-3xl font-bold tracking-tight bg-gradient-to-r from-gray-800 to-gray-600 bg-clip-text text-transparent dark:from-gray-100 dark:to-gray-300">
          Risk Management
        </h1>
      </div>
      
      <Tabs defaultValue="metrics" className="space-y-4">
        <TabsList>
          <TabsTrigger value="metrics">Risk Metrics</TabsTrigger>
          <TabsTrigger value="limits">Position Limits</TabsTrigger>
        </TabsList>
        <TabsContent value="metrics" className="space-y-4">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardHeader>
              <CardTitle className="text-gray-700 dark:text-gray-200">Portfolio Risk Analysis</CardTitle>
            </CardHeader>
            <CardContent className="grid gap-4 md:grid-cols-2 lg:grid-cols-4">
              {riskMetricsData.map((metric) => (
                <div key={metric.name} className="space-y-2">
                  <div className="text-sm font-medium text-muted-foreground">{metric.name}</div>
                  <div className="text-2xl font-bold">{metric.value}</div>
                </div>
              ))}
            </CardContent>
          </Card>
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardHeader>
              <CardTitle className="text-gray-700 dark:text-gray-200">Risk Distribution</CardTitle>
            </CardHeader>
            <CardContent>
              <ResponsiveContainer width="100%" height={300}>
                <BarChart data={riskMetricsData}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#e5e7eb" opacity={0.3} />
                  <XAxis 
                    dataKey="name" 
                    stroke="#888888"
                    fontSize={12}
                    tickLine={false}
                    axisLine={false}
                  />
                  <YAxis 
                    stroke="#888888"
                    fontSize={12}
                    tickLine={false}
                    axisLine={false}
                  />
                  <Tooltip 
                    contentStyle={{ 
                      backgroundColor: 'rgba(255, 255, 255, 0.8)',
                      borderRadius: '8px',
                      border: '1px solid #eaeaea',
                      boxShadow: '0 2px 4px rgba(0,0,0,0.05)'
                    }}
                    labelStyle={{ color: '#666' }}
                  />
                  <Bar 
                    dataKey="value" 
                    fill="#3B82F6"
                    radius={[4, 4, 0, 0]}
                    barSize={30}
                  />
                </BarChart>
              </ResponsiveContainer>
            </CardContent>
          </Card>
        </TabsContent>
        <TabsContent value="limits" className="space-y-4">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardHeader>
              <CardTitle className="text-gray-700 dark:text-gray-200">Position Limits</CardTitle>
            </CardHeader>
            <CardContent className="space-y-6">
              {positionLimitsData.map((position) => (
                <div key={position.asset} className="space-y-2">
                  <div className="flex justify-between">
                    <span className="text-sm font-medium text-muted-foreground">{position.asset}</span>
                    <span className="text-sm font-medium">{position.current} / {position.limit} USD</span>
                  </div>
                  <Progress value={(position.current / position.limit) * 100} />
                </div>
              ))}
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  )
}

