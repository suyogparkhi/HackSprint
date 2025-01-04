'use client'

import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { LineChart, Line, BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts'
import { DataTable } from "@/components/ui/data-table"
import { ColumnDef } from "@tanstack/react-table"

// Sample data for charts
const performanceData = [
  { date: '2023-01-01', return: 5.2 },
  { date: '2023-02-01', return: 3.8 },
  { date: '2023-03-01', return: -2.1 },
  { date: '2023-04-01', return: 6.7 },
  { date: '2023-05-01', return: 4.3 },
  { date: '2023-06-01', return: 1.9 },
]

const assetAllocationData = [
  { name: 'BTC', value: 40 },
  { name: 'ETH', value: 30 },
  { name: 'SOL', value: 15 },
  { name: 'ADA', value: 10 },
  { name: 'DOT', value: 5 },
]

// Sample data for trading history
const tradingHistoryData = [
  { id: 1, date: '2023-06-01', pair: 'BTC/USD', type: 'Buy', amount: 0.5, price: 30000 },
  { id: 2, date: '2023-06-02', pair: 'ETH/USD', type: 'Sell', amount: 2, price: 1800 },
  { id: 3, date: '2023-06-03', pair: 'SOL/USD', type: 'Buy', amount: 20, price: 20 },
  { id: 4, date: '2023-06-04', pair: 'BTC/USD', type: 'Sell', amount: 0.2, price: 31000 },
  { id: 5, date: '2023-06-05', pair: 'ETH/USD', type: 'Buy', amount: 1.5, price: 1850 },
]

// Column definition for the trading history table
const columns: ColumnDef<typeof tradingHistoryData[0]>[] = [
  {
    accessorKey: "date",
    header: "Date",
  },
  {
    accessorKey: "pair",
    header: "Trading Pair",
  },
  {
    accessorKey: "type",
    header: "Type",
  },
  {
    accessorKey: "amount",
    header: "Amount",
  },
  {
    accessorKey: "price",
    header: "Price (USD)",
  },
]

export default function AnalyticsPage() {
  return (
    <div className="grid gap-6">
      <div className="flex items-center justify-between">
        <h1 className="text-3xl font-bold tracking-tight bg-gradient-to-r from-gray-800 to-gray-600 bg-clip-text text-transparent dark:from-gray-100 dark:to-gray-300">
          Analytics Dashboard
        </h1>
        <p className="text-sm text-muted-foreground">Analyze your trading performance and portfolio</p>
      </div>
      
      <Tabs defaultValue="performance" className="space-y-4">
        <TabsList>
          <TabsTrigger value="performance">Performance</TabsTrigger>
          <TabsTrigger value="allocation">Asset Allocation</TabsTrigger>
          <TabsTrigger value="history">Trading History</TabsTrigger>
        </TabsList>
        
        <TabsContent value="performance" className="space-y-4">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardHeader>
              <CardTitle className="text-gray-700 dark:text-gray-200">Portfolio Performance</CardTitle>
            </CardHeader>
            <CardContent>
              <ResponsiveContainer width="100%" height={300}>
                <LineChart data={performanceData}>
                  <CartesianGrid strokeDasharray="3 3" />
                  <XAxis dataKey="date" />
                  <YAxis />
                  <Tooltip />
                  <Line type="monotone" dataKey="return" stroke="#4F6AF6" />
                </LineChart>
              </ResponsiveContainer>
            </CardContent>
          </Card>
        </TabsContent>
        
        <TabsContent value="allocation" className="space-y-4">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardHeader>
              <CardTitle className="text-gray-700 dark:text-gray-200">Asset Allocation</CardTitle>
            </CardHeader>
            <CardContent>
              <ResponsiveContainer width="100%" height={300}>
                <BarChart data={assetAllocationData}>
                  <CartesianGrid strokeDasharray="3 3" />
                  <XAxis dataKey="name" />
                  <YAxis />
                  <Tooltip />
                  <Bar dataKey="value" fill="#4F6AF6" />
                </BarChart>
              </ResponsiveContainer>
            </CardContent>
          </Card>
        </TabsContent>
        
        <TabsContent value="history" className="space-y-4">
          <Card className="backdrop-blur-sm bg-white/50 dark:bg-gray-900/50 border-0 shadow-[0_0_1px_rgba(0,0,0,0.1),0_2px_4px_rgba(0,0,0,0.05)]">
            <CardHeader>
              <CardTitle className="text-gray-700 dark:text-gray-200">Trading History</CardTitle>
            </CardHeader>
            <CardContent>
              <DataTable columns={columns} data={tradingHistoryData} />
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  )
}

