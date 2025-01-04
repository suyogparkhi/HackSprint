'use client'

import { Play, Pause, RefreshCw } from 'lucide-react'
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
import { Badge } from "@/components/ui/badge"
import { Switch } from "@/components/ui/switch"
import { Slider } from "@/components/ui/slider"

export default function AIAgentPage() {
  return (
    <div className="grid gap-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-3xl font-bold tracking-tight bg-gradient-to-r from-gray-800 to-gray-600 bg-clip-text text-transparent dark:from-gray-100 dark:to-gray-300">
            AI Trading Agent
          </h1>
          <p className="text-sm text-muted-foreground">Automated trading with ML-enhanced signals</p>
        </div>
        <div className="flex items-center gap-4">
          <Badge variant="secondary">Inactive</Badge>
          <Button size="icon" className="bg-[#4CAF50] hover:bg-[#45a049] text-white">
            <Play className="h-4 w-4" />
          </Button>
        </div>
      </div>
      
      <div className="grid gap-6 md:grid-cols-2">
        <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
          <CardHeader>
            <CardTitle className="text-gray-700 dark:text-gray-200">Strategy Configuration</CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="space-y-2">
              <Label className="text-gray-600 dark:text-gray-300">Trading Strategy</Label>
              <Select defaultValue="momentum">
                <SelectTrigger className="bg-white/80 dark:bg-gray-800/80">
                  <SelectValue placeholder="Select strategy" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="momentum">Momentum Trading</SelectItem>
                  <SelectItem value="mean-reversion">Mean Reversion</SelectItem>
                  <SelectItem value="breakout">Breakout Trading</SelectItem>
                </SelectContent>
              </Select>
            </div>
            <div className="space-y-2">
              <Label className="text-gray-600 dark:text-gray-300">Risk Level</Label>
              <Select defaultValue="moderate">
                <SelectTrigger className="bg-white/80 dark:bg-gray-800/80">
                  <SelectValue placeholder="Select risk level" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="conservative">Conservative</SelectItem>
                  <SelectItem value="moderate">Moderate</SelectItem>
                  <SelectItem value="aggressive">Aggressive</SelectItem>
                </SelectContent>
              </Select>
            </div>
            <div className="space-y-2">
              <Label className="text-gray-600 dark:text-gray-300">Auto-rebalance</Label>
              <div className="flex items-center space-x-2">
                <Switch id="auto-rebalance" />
                <Label htmlFor="auto-rebalance">Enable automatic portfolio rebalancing</Label>
              </div>
            </div>
          </CardContent>
        </Card>
        
        <div className="grid gap-4 sm:grid-cols-2">
          <Card className="bg-blue-100 dark:bg-blue-900/30">
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-blue-600 dark:text-blue-400">Total Trades</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">142</div>
            </CardContent>
          </Card>
          <Card className="bg-green-100 dark:bg-green-900/30">
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-green-600 dark:text-green-400">Success Rate</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">68.5%</div>
            </CardContent>
          </Card>
          <Card className="bg-purple-100 dark:bg-purple-900/30">
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-purple-600 dark:text-purple-400">Profit Today</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">$850.25</div>
            </CardContent>
          </Card>
          <Card className="bg-orange-100 dark:bg-orange-900/30">
            <CardHeader className="pb-2">
              <CardTitle className="text-sm font-medium text-orange-600 dark:text-orange-400">Active Positions</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">3</div>
            </CardContent>
          </Card>
        </div>
      </div>

      <Card className="backdrop-blur-sm bg-gradient-to-br from-white/80 to-white/50 dark:from-gray-900/80 dark:to-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
        <CardHeader>
          <CardTitle className="text-gray-700 dark:text-gray-200">Advanced AI Parameters</CardTitle>
        </CardHeader>
        <CardContent className="space-y-6">
          <div className="space-y-2">
            <Label className="text-gray-600 dark:text-gray-300">Position Size (% of Portfolio)</Label>
            <div className="px-2">
              <Slider defaultValue={[2]} max={10} step={0.1} />
            </div>
          </div>
          <div className="space-y-2">
            <Label className="text-gray-600 dark:text-gray-300">Max Daily Trades</Label>
            <div className="px-2">
              <Slider defaultValue={[10]} max={50} step={1} />
            </div>
          </div>
          <div className="space-y-2">
            <Label className="text-gray-600 dark:text-gray-300">Stop Loss (%)</Label>
            <div className="px-2">
              <Slider defaultValue={[2]} max={10} step={0.1} />
            </div>
          </div>
          <div className="space-y-2">
            <Label className="text-gray-600 dark:text-gray-300">Take Profit (%)</Label>
            <div className="px-2">
              <Slider defaultValue={[5]} max={20} step={0.1} />
            </div>
          </div>
          <Button className="w-full bg-[#2C3E50] hover:bg-[#34495E] text-white transition-all">
            <RefreshCw className="w-4 h-4 mr-2" /> Update AI Model
          </Button>
        </CardContent>
      </Card>
    </div>
  )
}

