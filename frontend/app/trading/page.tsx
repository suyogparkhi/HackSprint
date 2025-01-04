'use client'

import { useState } from "react"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts'

const data = [
  { time: '09:00', btcPrice: 45000, ethPrice: 2300, solPrice: 95 },
  { time: '10:00', btcPrice: 45500, ethPrice: 2320, solPrice: 96 },
  { time: '11:00', btcPrice: 45300, ethPrice: 2280, solPrice: 94 },
  { time: '12:00', btcPrice: 45800, ethPrice: 2350, solPrice: 97 },
  { time: '13:00', btcPrice: 46000, ethPrice: 2400, solPrice: 99 },
  { time: '14:00', btcPrice: 45700, ethPrice: 2380, solPrice: 98 },
  { time: '15:00', btcPrice: 46200, ethPrice: 2420, solPrice: 100 }
]

export default function TradingPage() {
  const [pair, setPair] = useState("BTC/USD")
  
  return (
    <div className="grid gap-6">
      <div className="flex items-center justify-between">
        <h1 className="text-3xl font-bold tracking-tight bg-gradient-to-r from-gray-800 to-gray-600 bg-clip-text text-transparent dark:from-gray-100 dark:to-gray-300">
          Trading Dashboard
        </h1>
        <p className="text-sm text-muted-foreground">Execute orders and monitor market trends</p>
      </div>
      
      <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
        <CardHeader>
          <CardTitle className="text-gray-700 dark:text-gray-200">Market Overview</CardTitle>
        </CardHeader>
        <CardContent>
          <ResponsiveContainer width="100%" height={300}>
            <LineChart data={data}>
              <CartesianGrid strokeDasharray="3 3" opacity={0.2} />
              <XAxis 
                dataKey="time" 
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
                tickFormatter={(value) => `$${value}`}
              />
              <Tooltip 
                contentStyle={{ 
                  backgroundColor: 'rgba(255, 255, 255, 0.8)',
                  borderRadius: '8px',
                  border: '1px solid #eaeaea'
                }}
                labelStyle={{ color: '#666' }}
              />
              <Line 
                type="monotone" 
                dataKey="btcPrice" 
                stroke="#FF9900" 
                strokeWidth={2}
                dot={false}
                name="BTC"
              />
              <Line 
                type="monotone" 
                dataKey="ethPrice" 
                stroke="#627EEA" 
                strokeWidth={2}
                dot={false}
                name="ETH"
              />
              <Line 
                type="monotone" 
                dataKey="solPrice" 
                stroke="#14F195" 
                strokeWidth={2}
                dot={false}
                name="SOL"
              />
            </LineChart>
          </ResponsiveContainer>
        </CardContent>
      </Card>
      
      <Tabs defaultValue="market" className="space-y-4">
        <TabsList>
          <TabsTrigger value="market">Market Order</TabsTrigger>
          <TabsTrigger value="limit">Limit Order</TabsTrigger>
        </TabsList>
        <TabsContent value="market">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardContent className="space-y-4 pt-6">
              <div className="space-y-2">
                <Label className="text-gray-600 dark:text-gray-300">Trading Pair</Label>
                <Select value={pair} onValueChange={setPair}>
                  <SelectTrigger className="bg-white/80 dark:bg-gray-800/80">
                    <SelectValue placeholder="Select pair" />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="BTC/USD">BTC/USD</SelectItem>
                    <SelectItem value="ETH/USD">ETH/USD</SelectItem>
                    <SelectItem value="SOL/USD">SOL/USD</SelectItem>
                  </SelectContent>
                </Select>
              </div>
              <div className="space-y-2">
                <Label className="text-gray-600 dark:text-gray-300">Amount (USD)</Label>
                <Input 
                  type="number" 
                  placeholder="Enter amount" 
                  className="bg-white/80 dark:bg-gray-800/80"
                />
              </div>
              <div className="grid grid-cols-2 gap-4">
                <Button className="bg-[#4CAF50] hover:bg-[#45a049] text-white transition-colors">
                  Buy
                </Button>
                <Button 
                  variant="outline"
                  className="border-[#F87171] text-[#F87171] hover:bg-[#F87171] hover:text-white transition-colors"
                >
                  Sell
                </Button>
              </div>
            </CardContent>
          </Card>
        </TabsContent>
        <TabsContent value="limit">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardContent className="space-y-4 pt-6">
              <div className="space-y-2">
                <Label className="text-gray-600 dark:text-gray-300">Trading Pair</Label>
                <Select value={pair} onValueChange={setPair}>
                  <SelectTrigger className="bg-white/80 dark:bg-gray-800/80">
                    <SelectValue placeholder="Select pair" />
                  </SelectTrigger>
                  <SelectContent>
                    <SelectItem value="BTC/USD">BTC/USD</SelectItem>
                    <SelectItem value="ETH/USD">ETH/USD</SelectItem>
                    <SelectItem value="SOL/USD">SOL/USD</SelectItem>
                  </SelectContent>
                </Select>
              </div>
              <div className="space-y-2">
                <Label className="text-gray-600 dark:text-gray-300">Price (USD)</Label>
                <Input 
                  type="number" 
                  placeholder="Enter price" 
                  className="bg-white/80 dark:bg-gray-800/80"
                />
              </div>
              <div className="space-y-2">
                <Label className="text-gray-600 dark:text-gray-300">Amount (USD)</Label>
                <Input 
                  type="number" 
                  placeholder="Enter amount" 
                  className="bg-white/80 dark:bg-gray-800/80"
                />
              </div>
              <div className="grid grid-cols-2 gap-4">
                <Button className="bg-[#4CAF50] hover:bg-[#45a049] text-white transition-colors">
                  Buy Limit
                </Button>
                <Button 
                  variant="outline"
                  className="border-[#F87171] text-[#F87171] hover:bg-[#F87171] hover:text-white transition-colors"
                >
                  Sell Limit
                </Button>
              </div>
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  )
}

